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

#ident "$XORP: xorp/bgp/main.cc,v 1.49 2008/01/04 03:15:20 pavlin Exp $"

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"

#include "libcomm/comm_api.h"

#include "libxipc/xrl_std_router.hh"

#include "bgp.hh"
#include "xrl_target.hh"


BGPMain *bgpmain;

void
terminate_main_loop(int sig)
{
    debug_msg("Signal %d\n", sig);
    UNUSED(sig);
    bgpmain->terminate();
}

int
main(int /*argc*/, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_level_set_verbose(XLOG_LEVEL_WARNING, XLOG_VERBOSE_HIGH);
    // Enable verbose tracing via configuration to increase the tracing level
//     xlog_level_set_verbose(XLOG_LEVEL_INFO, XLOG_VERBOSE_HIGH);
//     xlog_level_set_verbose(XLOG_LEVEL_TRACE, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    comm_init();

    try {
	EventLoop eventloop;

// 	signal(SIGINT, terminate_main_loop);

	BGPMain bgp(eventloop);
	bgpmain = &bgp;		// An external reference for the
				// signal handler.

	/*
	** By default assume there is a rib running.
	*/
 	bgp.register_ribname("rib");

	/*
	** Wait for our local configuration information and for the
	** FEA and RIB to start.
	*/
	while (bgp.get_xrl_target()->waiting() && bgp.run()) {
	    eventloop.run();
	}

	/*
	** Check we shouldn't be exiting.
	*/
 	if (!bgp.get_xrl_target()->done())
 	   bgp.main_loop();
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    comm_exit();
    debug_msg("Bye!\n");
    return 0;
}
