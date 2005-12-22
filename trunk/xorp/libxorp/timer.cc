// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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
//
// Portions of this code originally derived from:
// timer.{cc,hh} -- portable timers. Linux kernel module uses Linux timers
// Eddie Kohler
//
// Copyright (c) 1999-2000 Massachusetts Institute of Technology
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, subject to the conditions
// listed in the Click LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Click LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/libxorp/timer.cc,v 1.28 2005/12/21 09:42:57 bms Exp $"

#include "xorp.h"
#include "timer.hh"
#include "clock.hh"

// Implementation Notes:
//
// Event scheduling happens through the TimerList.  The TimerList is
// comprised of TimerNode's that contain an expiry time, a callback to
// make when the timer expires, and a thunk value to pass with the
// callback.  TimerNode's that are not scheduled are not part of the
// TimerList, but remain "associated" with their originating list so
// can be rescheduled by calling TimerNode methods (eg
// reschedule_after_ms()).
//
// User applications deal with XorpTimer objects that are pointers to
// TimerNodes.  There is some magic with XorpTimer objects: they enforce
// reference counting of TimerNodes.  When there are no XorpTimers
// referring to a particular TimerNode it is de-scheduled and
// destroyed.  This makes it difficult, though not impossible, for
// timer event callbacks to be invoked that pass invalid stack or heap
// memory into the callback.  Under normal usage we expect XorpTimer
// objects and the associated thunk values to have similar scope so
// they both exist and disappear at the same time.

//-----------------------------------------------------------------------------
// Constants

// ----------------------------------------------------------------------------
// TimerNode methods

TimerNode::TimerNode(TimerList* l, BasicTimerCallback cb)
    : _ref_cnt(0), _cb(cb), _list(l)
{
}

TimerNode::~TimerNode()
{
    unschedule();
}

void
TimerNode::add_ref()
{
    _ref_cnt++;
}

void
TimerNode::release_ref()
{
    if (--_ref_cnt <= 0)
	delete this;
}

void
TimerNode::expire(XorpTimer&, void*)
{
    // Implemented by children
}

bool
TimerNode::time_remaining(TimeVal& remain) const
{
    TimeVal now;

    assert(_list);
    _list->current_time(now);

    remain = expiry();
    if (remain <= now)
	remain = TimeVal::ZERO();
    else
	remain -= now;

    return (true);
}

void
TimerNode::unschedule()
{
    if (scheduled())
	_list->unschedule_node(this);
}

void
TimerNode::schedule_at(const TimeVal& t)
{
    assert(_list);
    unschedule();
    _expires = t;
    _list->schedule_node(this);
}

void
TimerNode::schedule_after(const TimeVal& wait)
{
    assert(_list);
    unschedule();

    TimeVal now;

    _list->current_time(now);
    _expires = now + wait;
    _list->schedule_node(this);
}

void
TimerNode::schedule_after_ms(int ms)
{
    assert(_list);
    unschedule();

    TimeVal now, interval(ms / 1000, (ms % 1000) * 1000);

    _list->current_time(now);
    _expires = now + interval;
    _list->schedule_node(this);
}

void
TimerNode::reschedule_after_ms(int ms)
{
    assert(_list);
    unschedule();

    TimeVal interval(ms / 1000, (ms % 1000) * 1000);

    _expires = _expires + interval;
    _list->schedule_node(this);
}

// ----------------------------------------------------------------------------
// Specialized Timers.  These are what the XorpTimer objects returned by
// the TimerList XorpTimer creation methods (e.g. TimerList::new_oneoff_at(), etc)
// actually refer to/

class OneoffTimerNode2 : public TimerNode {
public:
    OneoffTimerNode2(TimerList *l, const OneoffTimerCallback& cb)
	: TimerNode (l, callback(this, &OneoffTimerNode2::expire, (void*)0)),
	_cb(cb) {}
private:
    OneoffTimerCallback _cb;

    void expire(XorpTimer&, void*) {
	_cb->dispatch();
    }
};

class PeriodicTimerNode2 : public TimerNode {
public:
    PeriodicTimerNode2(TimerList *l, const PeriodicTimerCallback& cb, int ms)
	: TimerNode(l, callback(this, &PeriodicTimerNode2::expire, (void*)0)),
		    _cb(cb), _period_ms(ms) { }

private:
    PeriodicTimerCallback _cb;
    int _period_ms;

    void expire(XorpTimer& t, void*) {
	if (_cb->dispatch())
	    t.reschedule_after_ms(_period_ms);
    }
};


// ----------------------------------------------------------------------------
// TimerList implemention

static TimerList* the_timerlist = NULL;

TimerList::TimerList(ClockBase* clock)
    : Heap(true),
      _clock(clock), _observer(NULL)
{
    assert(the_timerlist == NULL);
    the_timerlist = this;
#ifdef HOST_OS_WINDOWS
    // timeBeginPeriod(1);	// requires WINMM.DLL
    _hirestimer = CreateWaitableTimer(NULL, TRUE, NULL);
    assert(_hirestimer != NULL);
#endif // HOST_OS_WINDOWS
}

TimerList::~TimerList()
{
#ifdef HOST_OS_WINDOWS
    if (_hirestimer != NULL)
	CloseHandle(_hirestimer);
    //timeEndPeriod(1);
#endif // HOST_OS_WINDOWS
    the_timerlist = NULL;
}

TimerList*
TimerList::instance()
{
    return the_timerlist;
}

void
TimerList::current_time(TimeVal& now) const
{
    _clock->current_time(now);
}

void
TimerList::advance_time()
{
    _clock->advance_time();
}

void
TimerList::system_gettimeofday(TimeVal* tv)
{
    TimerList* instance = TimerList::instance();
    if (!instance) {
	SystemClock s;
	TimerList timer = TimerList(&s);
	timer.system_gettimeofday(tv);
    } else {
	instance->advance_time();
	instance->current_time(*tv);
    }
}

/*
 * Call the underlying system's 'alertable wait' function.
 */
void
TimerList::system_sleep(const TimeVal& tv)
{
    TimerList* instance = TimerList::instance();
#ifdef HOST_OS_WINDOWS
    DWORD ms = tv.to_ms();
    if (ms == 0 || ms > 10) {
	Sleep(ms);
    } else {
	FILETIME ft;
	tv.copy_out(ft);
	assert(instance->_hirestimer != NULL);
	reinterpret_cast<LARGE_INTEGER *>(&ft)->QuadPart *= -1;
	SetWaitableTimer(instance->_hirestimer, (LARGE_INTEGER *)&ft, 0,
			 NULL, NULL, FALSE);
	WaitForSingleObject(instance->_hirestimer, INFINITE);
    }
#else // ! HOST_OS_WINDOWS
    if (tv.sec() > 0)
	sleep(tv.sec());
    if (tv.usec() > 0)
	usleep(tv.usec());
#endif // ! HOST_OS_WINDOWS

    instance->advance_time();
}


XorpTimer
TimerList::new_oneoff_at(const TimeVal& tv, const OneoffTimerCallback& cb)
{
    TimerNode* n = new OneoffTimerNode2(this, cb);
    n->schedule_at(tv);
    return XorpTimer(n);
}

XorpTimer
TimerList::new_oneoff_after(const TimeVal& wait,
			    const OneoffTimerCallback& cb)
{
    TimerNode* n = new OneoffTimerNode2(this, cb);

    n->schedule_after(wait);
    return XorpTimer(n);
}

XorpTimer
TimerList::new_oneoff_after_ms(int ms, const OneoffTimerCallback& cb)
{
    TimerNode* n = new OneoffTimerNode2(this, cb);
    n->schedule_after_ms(ms);
    return XorpTimer(n);
}

XorpTimer
TimerList::new_periodic(int interval, const PeriodicTimerCallback& cb)
{
    TimerNode* n = new PeriodicTimerNode2(this, cb, interval);
    n->schedule_after_ms(interval);
    return XorpTimer(n);
}

static void
set_flag_hook(bool* flag_ptr, bool to_value)
{
    assert(flag_ptr);
    *flag_ptr = to_value;
}

XorpTimer
TimerList::set_flag_at(const TimeVal& tv, bool *flag_ptr, bool to_value)
{
    assert(flag_ptr);
    *flag_ptr = false;
    return new_oneoff_at(tv, callback(set_flag_hook, flag_ptr, to_value));
}

XorpTimer
TimerList::set_flag_after(const TimeVal& wait, bool *flag_ptr, bool to_value)
{
    assert(flag_ptr);
    *flag_ptr = false;
    return new_oneoff_after(wait, callback(set_flag_hook, flag_ptr, to_value));
}

XorpTimer
TimerList::set_flag_after_ms(int ms, bool *flag_ptr, bool to_value)
{
    assert(flag_ptr);
    *flag_ptr = false;
    return new_oneoff_after_ms(ms, callback(set_flag_hook, flag_ptr, to_value));
}

void
TimerList::run()
{
    static const TimeVal WAY_BACK_GAP(15,0);

    TimeVal now;
    advance_time();
    current_time(now);

    struct heap_entry *n;
    while ((n = top()) != 0 && n->key <= now) {

	//
	// Throw a wobbly if we're a long way behind.
	//
	// We shouldn't write code that generates this message, it
	// means too long was spent in a timer callback or handling a
	// file descriptor event.  We can expect bad things (tm) to be
	// correlated with the appearance of this message.
	//
	TimeVal tardiness = now - n->key;
	if (tardiness > WAY_BACK_GAP) {
	    fprintf(stderr,
		    "========================================"
		    "=======================================\n");
	    fprintf(stderr,
		    "Timer Expiry *much* later than scheduled: "
		    "behind by %s seconds\n", tardiness.str().c_str());
	    fprintf(stderr,
		    "========================================"
		    "=======================================\n");
	}

	TimerNode *t = static_cast<TimerNode *>(n->object);
	pop();
	// _hook() requires a XorpTimer as first argument, we have
	// only a timernode, so we have to create a temporary
	// timer to invoke the hook.
	XorpTimer placeholder(t);
	t->expire(placeholder, 0);
	advance_time();
    }
}

bool
TimerList::empty() const
{
    acquire_lock();
    bool result = (top() == 0) ;
    release_lock();
    return result;
}

size_t
TimerList::size() const
{
    acquire_lock();
    size_t result = Heap::size();
    release_lock();
    return result;
}

bool
TimerList::get_next_delay(TimeVal& tv) const
{
    acquire_lock();
    struct heap_entry *t = top();
    release_lock();
    if (t == 0) {
	tv = TimeVal::MAXIMUM();
	return false;
    } else {
	TimeVal now;
	_clock->advance_time();
	_clock->current_time(now);
	if (t->key > now) {
	    // next event is in the future
	    tv = t->key - now ;
	} else {
	    // next event is already in the past, return 0.0
	    tv = TimeVal::ZERO();
	}
	return true;
    }
}

void
TimerList::schedule_node(TimerNode* n)
{
    acquire_lock();
    push(n->expiry(), n);
    release_lock();
    if (_observer) _observer->notify_scheduled(n->expiry());
}

void
TimerList::unschedule_node(TimerNode *n)
{
    acquire_lock();
    pop_obj(n);
    release_lock();
    if (_observer) _observer->notify_unscheduled(n->expiry());
}

void
TimerList::set_observer(TimerListObserverBase& obs)
{
    _observer = &obs;
    _observer->_observed = this;
}

void
TimerList::remove_observer()
{
    if (_observer) _observer->_observed = NULL;
    _observer = NULL;
}

TimerListObserverBase::~TimerListObserverBase()
{
    if (_observed) _observed->remove_observer();
}
