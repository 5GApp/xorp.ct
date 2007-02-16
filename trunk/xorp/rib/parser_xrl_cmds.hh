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

// $XORP: xorp/rib/parser_xrl_cmds.hh,v 1.15 2006/03/16 00:05:28 pavlin Exp $

#ifndef __RIB_PARSER_XRL_CMDS_HH__
#define __RIB_PARSER_XRL_CMDS_HH__

#include "xrl/interfaces/rib_xif.hh"

#include "parser.hh"


enum XrlCompletion {
    XRL_PENDING = 0,	// hack - corresponds with success for Parser::execute
    SUCCESS     = 1,
    XRL_FAILED  = -1
};

// Simple handler - most of RIB interface just returns true/false

static void
pass_fail_handler(const XrlError& e, XrlCompletion* c)
{
    if (e == XrlError::OKAY()) {
	*c = SUCCESS;
    } else {
	*c = XRL_FAILED;
	cerr << "Xrl Failed: " << e.str() << endl;
    }
    cout << "PassFailHander " << *c << endl;
}

// ----------------------------------------------------------------------------
// XRL Commands (NB fewer than number of direct commands)

class XrlRouteAddCommand : public RouteAddCommand {
public:
    XrlRouteAddCommand(EventLoop&	 e,
		       XrlRibV0p1Client& xrl_client,
		       XrlCompletion&	 completion)
	: RouteAddCommand(),
 	  _eventloop(e), _xrl_client(xrl_client), _completion(completion) {}

    int execute() {
	cout << "RouteAddCommand::execute " << _tablename << " " << _net.str()
	     << " " << _nexthop.str() << endl;

	_completion = XRL_PENDING;
	bool unicast = true, multicast = false;

	_xrl_client.send_add_route4(
	    "rib", _tablename, unicast, multicast, _net, _nexthop, _metric,
	    XrlAtomList(),	// XXX: no policy
	    callback(&pass_fail_handler, &_completion));

	return _completion;
    }

private:
    EventLoop&	      _eventloop;
    XrlRibV0p1Client& _xrl_client;
    XrlCompletion&    _completion;
};

class XrlRouteDeleteCommand : public RouteDeleteCommand {
public:
    XrlRouteDeleteCommand(EventLoop&		e,
			  XrlRibV0p1Client&	xrl_client,
			  XrlCompletion&	completion)
	: RouteDeleteCommand(),
	  _eventloop(e), _xrl_client(xrl_client), _completion(completion) {}

    int execute() {
	cout << "RouteDeleteCommand::execute " << _tablename << " "
	     << _net.str() << endl;

	_completion = XRL_PENDING;
	bool unicast = true, multicast = false;

	_xrl_client.send_delete_route4(
	    "rib", _tablename, unicast, multicast, _net,
	    callback(&pass_fail_handler, &_completion));

	return _completion;
    }

private:
    EventLoop&	      _eventloop;
    XrlRibV0p1Client& _xrl_client;
    XrlCompletion&     _completion;
};

class XrlAddIGPTableCommand : public AddIGPTableCommand {
public:
    XrlAddIGPTableCommand(EventLoop& 	    e,
			  XrlRibV0p1Client& xrl_client,
			  XrlCompletion&    completion) :
	AddIGPTableCommand(), _eventloop(e), _xrl_client(xrl_client),
	_completion(completion) {}

    int execute() {
	cout << "AddIGPTableCommand::execute " << _tablename << endl;

	_completion = XRL_PENDING;
	bool unicast = true, multicast = false;

	_xrl_client.send_add_igp_table4(
	    "rib", _tablename, "", "", unicast, multicast,
	    callback(&pass_fail_handler, &_completion));

	return _completion;
    }

private:
    EventLoop&	      _eventloop;
    XrlRibV0p1Client& _xrl_client;
    XrlCompletion&    _completion;
};

class XrlDeleteIGPTableCommand : public DeleteIGPTableCommand {
public:
    XrlDeleteIGPTableCommand(EventLoop&		e,
			     XrlRibV0p1Client&	xrl_client,
			     XrlCompletion&	completion) :
	DeleteIGPTableCommand(),
	_eventloop(e), _xrl_client(xrl_client), _completion(completion) {}

    int execute() {
	cout << "DeleteIGPTableCommand::execute " << _tablename << endl;

	_completion = XRL_PENDING;
	bool unicast = true, multicast = false;

	_xrl_client.send_delete_igp_table4(
	    "rib", _tablename, "", "", unicast, multicast,
	    callback(&pass_fail_handler, &_completion)
	    );

	return _completion;
    }

private:
    EventLoop&	      _eventloop;
    XrlRibV0p1Client& _xrl_client;
    XrlCompletion&    _completion;
};

class XrlAddEGPTableCommand : public AddEGPTableCommand {
public:
    XrlAddEGPTableCommand(EventLoop&	    e,
			  XrlRibV0p1Client& xrl_client,
			  XrlCompletion&    completion) :
	AddEGPTableCommand(), _eventloop(e), _xrl_client(xrl_client),
	_completion(completion) {}
    int execute() {
	cout << "AddEGPTableCommand::execute " << _tablename << endl;

	_completion = XRL_PENDING;

	bool unicast = true, multicast = false;
	_xrl_client.send_add_egp_table4(
	    "rib", _tablename, "", "", unicast, multicast,
	    callback(&pass_fail_handler, &_completion));

	return _completion;
    }

private:
    EventLoop&	      _eventloop;
    XrlRibV0p1Client& _xrl_client;
    XrlCompletion&    _completion;
};

class XrlDeleteEGPTableCommand : public DeleteEGPTableCommand {
public:
    XrlDeleteEGPTableCommand(EventLoop&		e,
			     XrlRibV0p1Client&	xrl_client,
			     XrlCompletion&	completion) :
	DeleteEGPTableCommand(), _eventloop(e), _xrl_client(xrl_client),
	_completion(completion) {}
    int execute() {
	cout << "DeleteEGPTableCommand::execute " << _tablename << endl;

	_completion = XRL_PENDING;
	bool unicast = true, multicast = false;
	_xrl_client.send_delete_egp_table4(
	    "rib", _tablename, "", "", unicast, multicast,
	    callback(&pass_fail_handler, &_completion));

	return _completion;
    }

private:
    EventLoop&	      _eventloop;
    XrlRibV0p1Client& _xrl_client;
    XrlCompletion&    _completion;
};

#endif // __RIB_PARSER_XRL_CMDS_HH__
