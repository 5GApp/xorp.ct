// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/libxorp/selector.cc,v 1.26 2005/08/30 01:02:47 pavlin Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxorp_module.h"

#include "xorp.h"
#include "debug.h"
#include "xorpfd.hh"
#include "selector.hh"
#include "timeval.hh"
#include "clock.hh"
#include "eventloop.hh"
#include "utility.h"
#include "xlog.h"

#ifdef	HOST_OS_WINDOWS
#define	XORP_FD_CAST(x)		((u_int)(x))
#else
#define	XORP_FD_CAST(x)		(x)
#endif

// ----------------------------------------------------------------------------
// Helper function to deal with translating between old and new
// I/O event notification mechanisms.

static SelectorMask
map_ioevent_to_selectormask(const IoEventType type)
{
    SelectorMask mask = SEL_NONE;

    // Convert new event type to legacy UNIX event mask used by SelectorList.
    switch (type) {
    case IOT_READ:
	mask = SEL_RD;
	break;
    case IOT_WRITE:
	mask = SEL_WR;
	break;
    case IOT_EXCEPTION:
	mask = SEL_EX;
	break;
    case IOT_ACCEPT:
	mask = SEL_RD;
	break;
    case IOT_CONNECT:
	mask = SEL_RD;
	break;
    case IOT_DISCONNECT:
	mask = SEL_EX;	// XXX: Disconnection isn't a distinct event in UNIX
	break;
    case IOT_ANY:
	mask = SEL_ALL;
	break;
    }
    return (mask);
}

// ----------------------------------------------------------------------------
// SelectorList::Node methods

inline
SelectorList::Node::Node()
{
    _mask[SEL_RD_IDX] = _mask[SEL_WR_IDX] = _mask[SEL_EX_IDX] = 0;
}

inline bool
SelectorList::Node::add_okay(SelectorMask m, IoEventType type,
			     const IoEventCb& cb)
{
    int i;

    // always OK to try to register for nothing
    if (!m)
	return true;

    // Sanity Checks

    // 0. We understand all bits in 'mode'
    assert((m & (SEL_RD | SEL_WR | SEL_EX)) == m);

    // 1. Check that bits in 'mode' are not already registered
    for (i = 0; i < SEL_MAX_IDX; i++) {
	if (_mask[i] & m) {
	    return false;
	}
    }

    // 2. If not already registered, find empty slot and add entry.
    // XXX: TODO: Determine if the node we're about to add is for
    // an accept event, so we know how to map it back.
    for (i = 0; i < SEL_MAX_IDX; i++) {
	if (!_mask[i]) {
	    _mask[i]	= m;
	    _cb[i]	= IoEventCb(cb);
	    _iot[i]	= type;
	    return true;
	}
    }

    assert(0);
    return false;
}

inline int
SelectorList::Node::run_hooks(SelectorMask m, XorpFd fd)
{
    int n = 0;

    /*
     * This is nasty.  We dispatch the callbacks here associated with
     * the file descriptor fd.  Unfortunately these callbacks can
     * manipulate the mask and callbacks associated with the
     * descriptor, ie the data change beneath our feet.  At no time do
     * we want to call a callback that has been removed so we can't
     * just copy the data before starting the dispatch process.  We do
     * not want to perform another callback here on a masked bit that
     * we have already done a callback on.  We therefore keep track of
     * the bits already matched with the variable already_matched.
     *
     * An alternate fix is to change the semantics of add_ioevent_cb so
     * there is one callback for each I/O event.
     *
     * Yet another alternative is to have an object, let's call it a
     * Selector that is a handle state of a file descriptor: ie an fd, a
     * mask, a callback and an enabled flag.  We would process the Selector
     * state individually.
     */
    SelectorMask already_matched = SelectorMask(0);

    for (int i = 0; i < SEL_MAX_IDX; i++) {
	SelectorMask match = SelectorMask(_mask[i] & m & ~already_matched);
	if (match) {
	    assert(_cb[i].is_empty() == false);
	    _cb[i]->dispatch(fd, _iot[i]);
	    n++;
	}
	already_matched = SelectorMask(already_matched | match);
    }
    return n;
}

inline void
SelectorList::Node::clear(SelectorMask zap)
{
    for (size_t i = 0; i < SEL_MAX_IDX; i++) {
	_mask[i] &= ~zap;
	if (_mask[i] == 0)
	    _cb[i].release();
    }
}

inline bool
SelectorList::Node::is_empty()
{
    return (_mask[SEL_RD_IDX] == _mask[SEL_WR_IDX] == _mask[SEL_EX_IDX] == 0);
}

// ----------------------------------------------------------------------------
// SelectorList implementation

SelectorList::SelectorList(ClockBase *clock)
    : _clock(clock), _observer(NULL)
#ifndef HOST_OS_WINDOWS
    , _maxfd(0), _descriptor_count(0)
#endif
{
    static_assert(SEL_RD == (1 << SEL_RD_IDX) && SEL_WR == (1 << SEL_WR_IDX)
		  && SEL_EX == (1 << SEL_EX_IDX) && SEL_MAX_IDX == 3);
    for (int i = 0; i < SEL_MAX_IDX; i++)
	FD_ZERO(&_fds[i]);
}

SelectorList::~SelectorList()
{
}

bool
SelectorList::add_ioevent_cb(XorpFd		   fd,
			   IoEventType		   type,
			   const IoEventCb& cb)
{
    SelectorMask mask = map_ioevent_to_selectormask(type);

    if (mask == 0) {
	XLOG_FATAL("SelectorList::add_ioevent_cb: attempt to add invalid event "
		   "type (type = %d)\n", type);
    }

    if (!fd.is_valid()) {
	XLOG_FATAL("SelectorList::add_ioevent_cb: attempt to add invalid file "
		   "descriptor (fd = %s)\n", fd.str().c_str());
    }

#ifdef HOST_OS_WINDOWS
    // Winsock's select() function will only allow you to select
    // on Winsock socket handles.
    if (!fd.is_socket()) {
	XLOG_ERROR("SelectorList::add_ioevent_cb: attempt to add object which "
		   "is not a socket (fd = %s)\n", fd.str().c_str());
    }
    if (_selector_entries[fd].add_okay(mask, type, cb) == false) {
	return false;
    }
#else // ! HOST_OS_WINDOWS
    bool resize = false;
    if (fd >= _maxfd) {
	_maxfd = fd;
	if ((size_t)fd >= _selector_entries.size()) {
	    _selector_entries.resize(fd + 32);
	    resize = true;
	}
    }
    bool no_selectors_with_fd = _selector_entries[fd].is_empty();
    if (_selector_entries[fd].add_okay(mask, type, cb) == false) {
	return false;
    }
    if (no_selectors_with_fd)
	_descriptor_count++;
#endif // ! HOST_OS_WINDOWS

    for (int i = 0; i < SEL_MAX_IDX; i++) {
	if (mask & (1 << i)) {
	    FD_SET(XORP_FD_CAST(fd), &_fds[i]);
	    if (_observer) _observer->notify_added(fd, mask);
	}
    }

    return true;
}

void
SelectorList::remove_ioevent_cb(XorpFd fd, IoEventType type)
{
#ifndef HOST_OS_WINDOWS
    if (fd < 0 || fd >= (int)_selector_entries.size()) {
	XLOG_ERROR("Attempting to remove fd = %d that is outside range of "
		   "file descriptors 0..%u", (int)fd,
		   XORP_UINT_CAST(_selector_entries.size()));
	return;
    }
#endif

    SelectorMask mask = map_ioevent_to_selectormask(type);

    for (int i = 0; i < SEL_MAX_IDX; i++) {
	if (mask & (1 << i) && FD_ISSET(fd, &_fds[i])) {
	    FD_CLR(XORP_FD_CAST(fd), &_fds[i]);
	    if (_observer)
		_observer->notify_removed(fd, ((SelectorMask) (1 << i)));
	}
    }
    _selector_entries[fd].clear(mask);
    if (_selector_entries[fd].is_empty()) {
	assert(FD_ISSET(fd, &_fds[SEL_RD_IDX]) == 0);
	assert(FD_ISSET(fd, &_fds[SEL_WR_IDX]) == 0);
	assert(FD_ISSET(fd, &_fds[SEL_EX_IDX]) == 0);
#ifndef HOST_OS_WINDOWS
	_descriptor_count--;
#endif
    }
}

int
SelectorList::wait_and_dispatch(TimeVal* timeout)
{
    fd_set testfds[SEL_MAX_IDX];
    int n = 0;

    memcpy(testfds, _fds, sizeof(_fds));

#ifdef HOST_OS_WINDOWS
    if (timeout != NULL && _selector_entries.empty()) {
	// Winsock's select() will not block if the FD lists are empty.
	SleepEx(timeout->to_ms(), TRUE);
    } else if (timeout == NULL || *timeout == TimeVal::MAXIMUM()) {
	n = ::select(-1,
		     &testfds[SEL_RD_IDX],
		     &testfds[SEL_WR_IDX],
		     &testfds[SEL_EX_IDX],
		     NULL);
    } else {
	struct timeval tv_to;
	timeout->copy_out(tv_to);
	n = ::select(-1,
		     &testfds[SEL_RD_IDX],
		     &testfds[SEL_WR_IDX],
		     &testfds[SEL_EX_IDX],
		     &tv_to);
    }

    _clock->advance_time();

    if (n < 0) {
	switch (errno) {
	case WSAENOTSOCK:
	    callback_bad_descriptors();
	    break;
	case WSAEINVAL:
	    XLOG_FATAL("Bad select argument (probably timeval)");
	    break;
	case WSAEINTR:
	    // The system call was interrupted by a signal, hence return
	    // immediately to the event loop without printing an error.
	    debug_msg("SelectorList::wait_and_dispatch() interrupted by a signal\n");
	    break;
	default:
	    XLOG_ERROR("SelectorList::wait_and_dispatch() failed: %d", WSAGetLastError());
	    break;
	}
	return 0;
    }

    for (map<XorpFd,Node>::iterator ii = _selector_entries.begin();
	 ii != _selector_entries.end();
	 ++ii) {
	int mask = 0;
	if (FD_ISSET(XORP_FD_CAST(ii->first), &testfds[SEL_RD_IDX])) {
	    mask |= SEL_RD;
	}
	if (FD_ISSET(XORP_FD_CAST(ii->first), &testfds[SEL_WR_IDX])) {
	    mask |= SEL_WR;
	}
	if (FD_ISSET(XORP_FD_CAST(ii->first), &testfds[SEL_EX_IDX])) {
	    mask |= SEL_EX;
	}
	if (mask) {
	    ii->second.run_hooks(SelectorMask(mask), ii->first);
	}
    }

#else // ! HOST_OS_WINDOWS

    if (timeout == 0 || *timeout == TimeVal::MAXIMUM()) {
	n = ::select(_maxfd + 1,
		     &testfds[SEL_RD_IDX],
		     &testfds[SEL_WR_IDX],
		     &testfds[SEL_EX_IDX],
		     0);
    } else {
	struct timeval tv_to;
	timeout->copy_out(tv_to);
	n = ::select(_maxfd + 1,
		     &testfds[SEL_RD_IDX],
		     &testfds[SEL_WR_IDX],
		     &testfds[SEL_EX_IDX],
		     &tv_to);
    }

    _clock->advance_time();

    if (n < 0) {
	switch (errno) {
	case EBADF:
	    callback_bad_descriptors();
	    break;
	case EINVAL:
	    XLOG_FATAL("Bad select argument (probably timeval)");
	    break;
	case EINTR:
	    // The system call was interrupted by a signal, hence return
	    // immediately to the event loop without printing an error.
	    debug_msg("SelectorList::wait_and_dispatch() interrupted by a signal\n");
	    break;
	default:
	    XLOG_ERROR("SelectorList::wait_and_dispatch() failed: %s", strerror(errno));
	    break;
	}
	return 0;
    }

    for (int fd = 0; fd <= _maxfd; fd++) {
	int mask = 0;
	if (FD_ISSET(fd, &testfds[SEL_RD_IDX])) {
	    mask |= SEL_RD;
	    FD_CLR(fd, &testfds[SEL_RD_IDX]);	// paranoia
	}
	if (FD_ISSET(fd, &testfds[SEL_WR_IDX])) {
	    mask |= SEL_WR;
	    FD_CLR(fd, &testfds[SEL_WR_IDX]);	// paranoia
	}
	if (FD_ISSET(fd, &testfds[SEL_EX_IDX])) {
	    mask |= SEL_EX;
	    FD_CLR(fd, &testfds[SEL_EX_IDX]);	// paranoia
	}
	if (mask) {
	    _selector_entries[fd].run_hooks(SelectorMask(mask), fd);
	}
    }

    for (int i = 0; i <= _maxfd; i++) {
	assert(!FD_ISSET(i, &testfds[SEL_RD_IDX]));	// paranoia
	assert(!FD_ISSET(i, &testfds[SEL_WR_IDX]));	// paranoia
	assert(!FD_ISSET(i, &testfds[SEL_EX_IDX]));	// paranoia
    }

#endif // ! HOST_OS_WINDOWS

    return n;
}

int
SelectorList::wait_and_dispatch(int millisecs)
{
    TimeVal t(millisecs / 1000, (millisecs % 1000) * 1000);
    return wait_and_dispatch(&t);
}

void
SelectorList::get_fd_set(SelectorMask selected_mask, fd_set& fds) const
{
    if ((SEL_RD != selected_mask) && (SEL_WR != selected_mask) &&
	(SEL_EX != selected_mask)) return;
    if (SEL_RD == selected_mask) fds = _fds [SEL_RD_IDX];
    if (SEL_WR == selected_mask) fds = _fds [SEL_WR_IDX];
    if (SEL_EX == selected_mask) fds = _fds [SEL_EX_IDX];
    return;
}

#ifndef HOST_OS_WINDOWS
int
SelectorList::get_max_fd() const
{
    return _maxfd;
}
#endif // ! HOST_OS_WINDOWS

void
SelectorList::callback_bad_descriptors()
{
#ifndef HOST_OS_WINDOWS
    int bc = 0;	/* bad descriptor count */

    for (int fd = 0; fd <= _maxfd; fd++) {
	if (_selector_entries[fd].is_empty() == true)
	    continue;
	/* Check fd is valid.  NB we call fstat without a stat struct to
	 * avoid copy.  fstat will always fail as a result.
	 */
	fstat(fd, 0);
	if (errno == EBADF) {
	    /* Force callbacks, should force read/writes that fail and
	     * client should remove descriptor from list. */
	    XLOG_ERROR("SelectorList found file descriptor %d no longer "
		       "valid.", fd);
	    _selector_entries[fd].run_hooks(SEL_ALL, fd);
	    bc++;
	}
    }
    /* Assert should only fail if OS checks stat struct before fd (??) */
    assert(bc != 0);
#endif // ! HOST_OS_WINDOWS
}

void
SelectorList::set_observer(SelectorListObserverBase& obs)
{
    _observer = &obs;
    _observer->_observed = this;
    return;
}

void
SelectorList::remove_observer()
{
    if (_observer) _observer->_observed = NULL;
    _observer = NULL;
    return;
}

SelectorListObserverBase::~SelectorListObserverBase()
{
    if (_observed) _observed->remove_observer();
}

