// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/finder_xrl_queue.cc,v 1.8 2006/03/16 00:04:17 pavlin Exp $"

#include "finder_module.h"
#include "libxorp/debug.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "finder_messenger.hh"
#include "finder_xrl_queue.hh"

FinderXrlCommandQueue::FinderXrlCommandQueue(FinderMessengerBase* messenger)
    : _m(messenger), _pending(false)
{
}

FinderXrlCommandQueue::FinderXrlCommandQueue(const FinderXrlCommandQueue& oq)
    : _m(oq._m), _pending(oq._pending)
{
    XLOG_ASSERT(oq._cmds.empty());
    XLOG_ASSERT(oq._pending == false);
}

FinderXrlCommandQueue::~FinderXrlCommandQueue()
{
}

inline EventLoop&
FinderXrlCommandQueue::eventloop()
{
    return _m->eventloop();
}

inline void
FinderXrlCommandQueue::push()
{
    debug_msg("push\n");
    if (false == _pending && _cmds.empty() == false&&
	_dispatcher.scheduled() == false) {
	_dispatcher = eventloop().new_oneoff_after_ms(0,
			callback(this, &FinderXrlCommandQueue::dispatch_one));
    }
}

void
FinderXrlCommandQueue::dispatch_one()
{
    debug_msg("dispatch_one\n");
    XLOG_ASSERT(_cmds.empty() == false);
    _cmds.front()->dispatch();
    _pending = true;
}

void
FinderXrlCommandQueue::enqueue(const FinderXrlCommandQueue::Command& cmd)
{
    debug_msg("enqueue\n");
    _cmds.push_back(cmd);
    push();
}

void
FinderXrlCommandQueue::crank()
{
    debug_msg("crank\n");
    XLOG_ASSERT(_pending == true);
    _cmds.pop_front();
    _pending = false;
    push();
}

void
FinderXrlCommandQueue::kill_messenger()
{
    debug_msg("killing messenger\n");
    delete _m;
}
