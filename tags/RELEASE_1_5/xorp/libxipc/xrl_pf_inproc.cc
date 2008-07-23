// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxipc/xrl_pf_inproc.cc,v 1.33 2008/01/04 03:16:28 pavlin Exp $"

#include "xrl_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/c_format.hh"

#include <map>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "xrl_error.hh"
#include "xrl_pf_inproc.hh"
#include "xrl_dispatcher.hh"


// ----------------------------------------------------------------------------
// InProc is "intra-process" - a minimalist and simple direct call transport
// mechanism for XRLs.
//
// Resolved names have protocol name "inproc" and specify addresses as
// "host/pid.iid" where pid is process id and iid is instance id (number).
// The latter being used to differentiate when multiple listeners exist
// within a single process.
//

// ----------------------------------------------------------------------------
// Constants

static char INPROC_PROTOCOL_NAME[] = "inproc";

const char* XrlPFInProcSender::_protocol = INPROC_PROTOCOL_NAME;
const char* XrlPFInProcListener::_protocol = INPROC_PROTOCOL_NAME;

// Glue for Sender and Listener rendez-vous
static XrlPFInProcListener* get_inproc_listener(uint32_t instance_no);

// ----------------------------------------------------------------------------
// Utility Functions

static const string
this_host()
{
#ifdef HOST_OS_WINDOWS
    char buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD bufsiz;

    bufsiz = sizeof(buffer);
    if (0 == GetComputerNameExA(ComputerNamePhysicalDnsFullyQualified,
				buffer, &bufsiz)) {
	XLOG_FATAL("Could not obtain local computer name.");
    }
    return string(buffer);
#else
    char buffer[MAXHOSTNAMELEN + 1];
    buffer[MAXHOSTNAMELEN] ='\0';
    gethostname(buffer, MAXHOSTNAMELEN);
    return string(buffer);
#endif
}

bool
split_inproc_address(const char* address,
		     string& host, uint32_t& pid, uint32_t& iid)
{
    const char *p;

    p = address;
    for (;;) {
	if (*p == '\0') {
	    // unexpected end of input
	    return false;
	} else if (*p == ':') {
	    break;
	}
	p++;
    }
    host = string(address, p - address);
    p++;

    pid = 0;
    while (xorp_isdigit(*p)) {
	pid *= 10;
	pid += *p - '0';
	p++;
    }

    if (*p != '.')
	return false;
    p++;

    iid = 0;
    while (xorp_isdigit(*p)) {
	iid *= 10;
	iid += *p - '0';
	p++;
    }
    return (*p == '\0');
}

// ----------------------------------------------------------------------------
// XrlPFInProcSender

XrlPFInProcSender::XrlPFInProcSender(EventLoop& e, const char* address)
    throw (XrlPFConstructorError)
    : XrlPFSender(e, address)
{
    string hname;
    uint32_t pid, iid;

    if (split_inproc_address(address, hname, pid, iid) == false) {
	xorp_throw(XrlPFConstructorError,
		   c_format("Invalid address: %s", address));
    } else if (hname != this_host()) {
	xorp_throw(XrlPFConstructorError,
		   c_format("Wrong host: %s != %s",
			    hname.c_str(), this_host().c_str()));
    } else if (pid != (uint32_t)getpid()) {
	xorp_throw(XrlPFConstructorError, "Bad process id");
    }
    _listener_no = iid;
    _depth = new uint32_t(0);
}

XrlPFInProcSender::~XrlPFInProcSender()
{}

bool
XrlPFInProcSender::send(const Xrl&		x,
			bool			direct_call,
			const SendCallback&	cb)
{
    XrlPFInProcListener *l = get_inproc_listener(_listener_no);

    // Check stack depth.  This is a bit adhoc, but the issue is that
    // the inproc makes the dispatch and invokes the callback in this
    // send function.  There is zero delay.  So code that sends one
    // XRL from the callback of another in a long chains may end up
    // exhausting the stack.  We use depth as a counter.  And there's
    // the final subtlety that one of those callback may delete the
    // sender so we can't access member variables directly after the
    // callback returns, but we can access a reference pointer if we
    // have a reference!!!
    ref_ptr<uint32_t> depth = _depth;

    *depth = *depth + 1;
    if (*depth > 1) {
	if (direct_call == true) {
	    *depth = *depth - 1;
	    return false;
	} else {
	    cb->dispatch(XrlError(SEND_FAILED, "RESOURCES!"), 0);
	    *depth = *depth - 1;
	    return true;
	}
    }

    if (l == NULL) {
	if (direct_call) {
	    *depth = *depth - 1;
	    return false;
	} else {
	    debug_msg("XrlPFInProcSender::send() no listener (id %d)\n",
		      XORP_INT_CAST(_listener_no));
	    cb->dispatch(XrlError::SEND_FAILED(), 0);
	    *depth = *depth - 1;
	    return true;
	}
    }

    const XrlDispatcher *d = l->dispatcher();
    if (d == NULL) {
	if (direct_call) {
	    *depth = *depth - 1;
	    return false;
	} else {
	    debug_msg("XrlPFInProcSender::send() no dispatcher (id %d)\n",
		      XORP_INT_CAST(_listener_no));
	    cb->dispatch(XrlError::SEND_FAILED(), 0);
	}
	*depth = *depth - 1;
	return true;
    }

    XrlArgs reply;
    const XrlError e = d->dispatch_xrl(x.command(), x.args(), reply);
    cb->dispatch(e, (e == XrlError::OKAY()) ? &reply : 0);

    *depth = *depth - 1;
    return true;
}

const char*
XrlPFInProcSender::protocol() const
{
    return _protocol;
}

bool
XrlPFInProcSender::alive() const
{
    return true;
}

// ----------------------------------------------------------------------------
// XrlPFInProcListener instance management

static map<uint32_t, XrlPFInProcListener*> listeners;

static XrlPFInProcListener*
get_inproc_listener(uint32_t instance_no)
{
    map<uint32_t, XrlPFInProcListener*>::iterator i;
    debug_msg("getting -> size %u\n", XORP_UINT_CAST(listeners.size()));
    i = listeners.find(instance_no);
    return (i == listeners.end()) ? 0 : i->second;
}

static void
add_inproc_listener(uint32_t instance_no, XrlPFInProcListener* l)
{
    assert(get_inproc_listener(instance_no) == 0);
    debug_msg("adding no %d size %u\n", XORP_INT_CAST(instance_no),
	      XORP_UINT_CAST(listeners.size()));
    listeners[instance_no] = l;
}

static void
remove_inproc_listener(uint32_t instance_no)
{
    assert(get_inproc_listener(instance_no) != 0);
    listeners.erase(instance_no);
    debug_msg("Removing listener %d\n", XORP_INT_CAST(instance_no));
}

// ----------------------------------------------------------------------------
// XrlPFInProcListener

uint32_t XrlPFInProcListener::_next_instance_no;

XrlPFInProcListener::XrlPFInProcListener(EventLoop& e, XrlDispatcher* xr)
    throw (XrlPFConstructorError)
    : XrlPFListener(e, xr)
{
    _instance_no = _next_instance_no ++;

    _address = this_host() + c_format(":%d.%d", XORP_INT_CAST(getpid()),
				      XORP_INT_CAST(_instance_no));
    add_inproc_listener(_instance_no, this);
}

XrlPFInProcListener::~XrlPFInProcListener()
{
    remove_inproc_listener(_instance_no);
}

bool
XrlPFInProcListener::response_pending() const
{
    // Responses are immediate for InProc
    return false;
}
