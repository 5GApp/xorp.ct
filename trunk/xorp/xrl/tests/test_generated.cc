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

#ident "$XORP: xorp/xrl/tests/test_generated.cc,v 1.17 2006/10/12 01:25:15 pavlin Exp $"

#include "libxipc/ipc_module.h"

#include "libxorp/xorp.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"

#include <iostream>

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"
#include "libxipc/xrl_pf_sudp.hh"

#include "test_xifs.hh"
#include "test_tgt.hh"


static const char* g_tgt_name = "test_tgt";
static const char* g_clnt_name = "test_clnt";

static void
run_test()
{
    EventLoop e;
    FinderServer finder(e, FinderConstants::FINDER_DEFAULT_HOST(),
			FinderConstants::FINDER_DEFAULT_PORT());

    // Configure target for test
    XrlStdRouter tgt_router(e, g_tgt_name, finder.addr(), finder.port());
    XrlTestTarget tgt_target(&tgt_router);

    // Configure client for test
    XrlRouter clnt_router(e, g_clnt_name, finder.addr(), finder.port());

    bool timeout = false;
    XorpTimer t = e.set_flag_after_ms(2000, &timeout);
    while (tgt_router.ready() == false && clnt_router.ready() == false)
	e.run();

    if (timeout) {
	fprintf(stderr, "Timed out connecting to finder\n");
	exit(-1);
    }

    while (timeout == false)
	e.run();

    // Run test
    fprintf(stderr, "common xif methods\n");
    try_common_xif_methods(&e, &clnt_router, tgt_router.name().c_str());
    fprintf(stderr, "test xif methods\n");
    try_test_xif_methods(&e, &clnt_router, tgt_router.name().c_str());
}

int main(int /* argc */, const char* argv[])
{
    xlog_init(argv[0], 0);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	run_test();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    xlog_stop();
    xlog_exit();

    return 0;
}
