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

#ident "$XORP: xorp/bgp/tools/print_routes_main.cc,v 1.10 2006/07/12 00:22:00 atanu Exp $"

#include "print_routes.hh"
#include "bgp/aspath.hh"
#include "bgp/path_attribute.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

void usage()
{
    fprintf(stderr,
	    "Usage: print_routes [-4 -6 -u -m -s -v -p <prefix>]"
	    " [-l <lines>]" 
	    " [-i <repeat_interval>]\n"
	    "-4 IPv4\n"
	    "-6 IPv6\n"
	    "-p <prefix>\n"
	    "-u Unicast\n"
	    "-m Multicast\n"
	    "-s summary output\n"
	    "-v enables verbose output\n"
	    );
}

int main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    string prefix;
    bool ipv4, ipv6, unicast, multicast;
    ipv4 = ipv6 = unicast = multicast = false;
    int lines = -1;

    PrintRoutes<IPv4>::detail_t verbose_ipv4 = PrintRoutes<IPv4>::NORMAL;
    PrintRoutes<IPv6>::detail_t verbose_ipv6 = PrintRoutes<IPv6>::NORMAL;
    int c;
    int interval = -1;
    while ((c = getopt(argc, argv, "46p:umvi:l:")) != -1) {
	switch (c) {
	case '4':
	    ipv4 = true;
	    break;
	case '6':
	    ipv6 = true;
	    break;
	case 'p':
	    prefix = optarg;
	    break;
	case 'u':
	    unicast = true;
	    break;
	case 'm':
	    multicast = true;
	    break;
	case 'l':
	    lines = atoi(optarg);
	    break;
	case 's':
	    verbose_ipv4 = PrintRoutes<IPv4>::SUMMARY;
	    verbose_ipv6 = PrintRoutes<IPv6>::SUMMARY;
	    break;
	case 'v':
	    verbose_ipv4 = PrintRoutes<IPv4>::DETAIL;
	    verbose_ipv6 = PrintRoutes<IPv6>::DETAIL;
	    break;
	case 'i':
	    interval = atoi(optarg);
	    break;
	default:
	    usage();
	    return -1;
	}
    }

    if (ipv4 == false && ipv6 == false)
	ipv4 = true;

    if (unicast == false && multicast == false)
	unicast = true;

    try {
	if (ipv4) {
	    IPNet<IPv4> net;
	    if ("" != prefix) {
		IPNet<IPv4> subnet(prefix.c_str());
		net = subnet;
	    }
	    PrintRoutes<IPv4> route_printer(verbose_ipv4, interval, net,
					    unicast, multicast, lines);
	}
	if (ipv6) {
	    IPNet<IPv6> net;
	    if ("" != prefix) {
		IPNet<IPv6> subnet(prefix.c_str());
		net = subnet;
	    }
	    PrintRoutes<IPv6> route_printer(verbose_ipv6, interval, net,
					    unicast, multicast, lines);
	}
	    
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}

