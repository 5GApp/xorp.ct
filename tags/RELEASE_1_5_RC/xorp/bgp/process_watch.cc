// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/process_watch.cc,v 1.16 2007/02/16 22:45:15 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/exceptions.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"

#include "exceptions.hh"
#include "process_watch.hh"

ProcessWatch::ProcessWatch(XrlStdRouter *xrl_router, EventLoop& eventloop,
			   const char *bgp_mib_name, TerminateCallback cb) :
    _eventloop(eventloop), _shutdown(cb), _fea(false), _rib(false)
{
	
    /*
    ** Register interest in the fea and rib and snmp trap handler.
    */
    XrlFinderEventNotifierV0p1Client finder(xrl_router);
    finder.send_register_class_event_interest("finder",
	xrl_router->instance_name(), "fea",
	    callback(this, &ProcessWatch::interest_callback));
    finder.send_register_class_event_interest("finder",
	xrl_router->instance_name(), "rib",
	    callback(this, &ProcessWatch::interest_callback));
    finder.send_register_class_event_interest("finder",
	xrl_router->instance_name(), bgp_mib_name,
	    callback(this, &ProcessWatch::interest_callback));
}

void
ProcessWatch::interest_callback(const XrlError& error)
{
    debug_msg("callback %s\n", error.str().c_str());

    if(XrlError::OKAY() != error.error_code()) {
	XLOG_FATAL("callback: %s",  error.str().c_str());
    }
}

void
ProcessWatch::birth(const string& target_class, const string& target_instance)
{
    debug_msg("birth: %s %s\n", target_class.c_str(), target_instance.c_str());
	
    if(target_class == "fea" && false == _fea) {
	_fea = true;
	_fea_instance = target_instance;
    } else if(target_class == "rib" && false == _rib) {
	_rib = true;
	_rib_instance = target_instance;
    } else
	add_target(target_class, target_instance);
}

void 
ProcessWatch::death(const string& target_class, const string& target_instance)
{
    debug_msg("death: %s %s\n", target_class.c_str(), target_instance.c_str());

    if (_fea_instance == target_instance) {
	XLOG_ERROR("The fea died");
	::exit(-1);
    } else if (_rib_instance == target_instance) {
	XLOG_ERROR("The rib died");
	start_kill_timer();
	_shutdown->dispatch();
    } else
	remove_target(target_class, target_instance);
}

void
ProcessWatch::finder_death(const char *file, const int lineno)
{
    XLOG_ERROR("The finder has died BGP process exiting called from %s:%d",
	       file, lineno);

    start_kill_timer();
    xorp_throw(NoFinder, "");
}

void
ProcessWatch::start_kill_timer()
{
    _shutdown_timer = _eventloop.
	new_oneoff_after_ms(1000 * 10, ::callback(::exit, -1));
}

bool
ProcessWatch::ready() const
{
    return _fea && _rib;
}

bool
ProcessWatch::target_exists(const string& target) const
{
    debug_msg("target_exists: %s\n", target.c_str());

    list<Process>::const_iterator i;
    for(i = _processes.begin(); i != _processes.end(); i++)
	if(target == i->_target_class) {
	    debug_msg("target: %s found\n", target.c_str());
	    return true;
	}

    debug_msg("target %s not found\n", target.c_str());

    return false;
}

void 
ProcessWatch::add_target(const string& target_class,
			  const string& target_instance)
{
    debug_msg("add_target: %s %s\n", target_class.c_str(),
	      target_instance.c_str());

    _processes.push_back(Process(target_class, target_instance));
}

void 
ProcessWatch::remove_target(const string& target_class,
			     const string& target_instance)
{
    debug_msg("remove_target: %s %s\n", target_class.c_str(),
	      target_instance.c_str());

    list<Process>::iterator i;
    for(i = _processes.begin(); i != _processes.end(); i++)
	if(target_class == i->_target_class &&
	   target_instance == i->_target_instance) {
	    _processes.erase(i);
	    return;
	}

    XLOG_FATAL("unknown target %s %s", target_class.c_str(),
	       target_instance.c_str());
}
