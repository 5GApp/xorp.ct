// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net





#include "rip_module.h"
#include "libxorp/xlog.h"

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"

#include "port.hh"
#include "peer.hh"
#include "route_db.hh"
#include "system.hh"

#include "test_utils.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_timers";
static const char *program_description  = "Test RIP timers and route scan";
static const char *program_version_id   = "0.1";
static const char *program_date         = "March, 2003";
static const char *program_copyright    = "See file LICENSE";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";

static const uint32_t N_TEST_ROUTES = 32000;

static bool
print_twirl(void)
{
    static const char t[] = { '\\', '|', '/', '-' };
    static const size_t nt = sizeof(t) / sizeof(t[0]);
    static size_t n = 0;
    static char erase = '\0';

    printf("%c%c", erase, t[n % nt]); fflush(stdout);
    n++;
    erase = '\b';
    return true;
}


// ----------------------------------------------------------------------------
template <typename A>
struct Address {
    static A me();
    static A peer();
};

template <>
IPv4 Address<IPv4>::me()		{ return IPv4("10.0.0.1"); }

template <>
IPv4 Address<IPv4>::peer()		{ return IPv4("10.10.0.1"); }

template <>
IPv6 Address<IPv6>::me()		{ return IPv6("10::1"); }

template <>
IPv6 Address<IPv6>::peer()		{ return IPv6("1010::1"); }


// ----------------------------------------------------------------------------
// Spoof Port that supports just a single Peer
//

template <typename A>
class SpoofPort : public Port<A> {
public:
    SpoofPort(PortManagerBase<A>& pm, A addr) : Port<A>(pm)
    {
	this->_peers.push_back(new Peer<A>(*this, addr));
	verbose_log("Constructing SpoofPort<IPv%u> instance\n",
		    XORP_UINT_CAST(A::ip_version()));
    }
    ~SpoofPort()
    {
	verbose_log("Destructing SpoofPort<IPv%u> instance\n",
		    XORP_UINT_CAST(A::ip_version()));
	while (this->_peers.empty() == false) {
	    delete this->_peers.front();
	    this->_peers.pop_front();
	}
    }
};

// ----------------------------------------------------------------------------
// Spoof Port Manager instance support a single Spoof Port which in turn
// contains a single Peer.
//

template <typename A>
class SpoofPortManager : public PortManagerBase<A> {
public:
    SpoofPortManager(System<A>& s, const IfMgrIfTree& iftree)
	: PortManagerBase<A>(s, iftree)
    {
	this->_ports.push_back(new SpoofPort<A>(*this, Address<A>::me()));
    }

    ~SpoofPortManager()
    {
	while (!this->_ports.empty()) {
	    delete this->_ports.front();
	    this->_ports.pop_front();
	}
    }

    Port<A>* the_port()
    {
	XLOG_ASSERT(this->_ports.size() == 1);
	return this->_ports.front();
    }

    Peer<A>* the_peer()
    {
	XLOG_ASSERT(this->_ports.size() == 1);
	XLOG_ASSERT(this->_ports.front()->peers().size() == 1);
	return this->_ports.front()->peers().front();
    }
};




//----------------------------------------------------------------------------
// The test

static const IfMgrIfTree ift_dummy = IfMgrIfTree();

template <typename A>
static int
test_main()
{
    string ifname, vifname;		// XXX: not set, because not needed
    const uint32_t n_routes = N_TEST_ROUTES;
    set<IPNet<A> > nets;
    make_nets(nets, n_routes);

    EventLoop		e;
    System<A>		rip_system(e);
    SpoofPortManager<A>	spm(rip_system, ift_dummy);

    RouteDB<A>& 	rdb  = rip_system.route_db();
    Peer<A>* 		peer = spm.the_peer();

    // Fix up time constants for the impatient.
    spm.the_port()->constants().set_expiry_secs(3);
    spm.the_port()->constants().set_deletion_secs(2);

    for_each(nets.begin(), nets.end(),
	     RouteInjector<A>(rdb, Address<A>::me(), ifname, vifname, 1,
			      peer));

    if (peer->route_count() != n_routes) {
	verbose_log("Routes lost (count %u != %u)\n",
		    XORP_UINT_CAST(peer->route_count()),
		    XORP_UINT_CAST(n_routes));
	return 1;
    }

    e.timer_list().advance_time(); // XXX

    {
	// Quick route dump test
	TimeVal t1, t2;
	e.current_time(t1);
	vector<typename RouteDB<A>::ConstDBRouteEntry> l;
	rdb.dump_routes(l);
	e.timer_list().advance_time(); // XXX
	e.current_time(t2);
	t2 -= t1;
	fprintf(stderr, "route db route dump took %d.%06d seconds\n",
		t2.sec(), t2.usec());
    }
    {
	// Quick route dump test
	TimeVal t1, t2;
	e.current_time(t1);
	vector<const typename RouteEntryOrigin<A>::Route*> l;
	peer->dump_routes(l);
	e.timer_list().advance_time(); // XXX
	e.current_time(t2);
	t2 -= t1;
	fprintf(stderr, "peer route dump took %d.%06d seconds\n",
		t2.sec(), t2.usec());
    }
    {
	// Quick route dump test
	TimeVal t1, t2;
	e.current_time(t1);
	vector<typename RouteDB<A>::ConstDBRouteEntry> l;
	rdb.dump_routes(l);
	e.timer_list().advance_time(); // XXX
	e.current_time(t2);
	t2 -= t1;
	fprintf(stderr, "route db route dump took %d.%06d seconds\n",
		t2.sec(), t2.usec());
    }

    const PortTimerConstants& ptc = peer->port().constants();
    uint32_t timeout_secs = ptc.expiry_secs() + ptc.deletion_secs() + 5;

    XorpTimer twirl;
    if (verbose())
	twirl = e.new_periodic_ms(250, callback(print_twirl));

    bool timeout = false;
    XorpTimer t = e.set_flag_after_ms(1000 * timeout_secs, &timeout);

    while (timeout == false) {
	size_t route_count = peer->route_count();
	verbose_log("Route count = %u\n", XORP_UINT_CAST(route_count));
	if (route_count == 0)
	    break;
	e.run();
    }

    if (peer->route_count()) {
	verbose_log("Routes did not clean up\n");
	return 1;
    }

    return 0;
}


/**
 * Print program info to output stream.
 *
 * @param stream the output stream the print the program info to.
 */
static void
print_program_info(FILE *stream)
{
    fprintf(stream, "Name:          %s\n", program_name);
    fprintf(stream, "Description:   %s\n", program_description);
    fprintf(stream, "Version:       %s\n", program_version_id);
    fprintf(stream, "Date:          %s\n", program_date);
    fprintf(stream, "Copyright:     %s\n", program_copyright);
    fprintf(stream, "Return:        %s\n", program_return_value);
}

/**
 * Print program usage information to the stderr.
 *
 * @param progname the name of the program.
 */
static void
usage(const char* progname)
{
    print_program_info(stderr);
    fprintf(stderr, "usage: %s [-v] [-h]\n", progname);
    fprintf(stderr, "       -h          : usage (this message)\n");
    fprintf(stderr, "       -v          : verbose output\n");
}

int
main(int argc, char* const argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    int ch;
    while ((ch = getopt(argc, argv, "hv")) != -1) {
        switch (ch) {
        case 'v':
            set_verbose(true);
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
            xlog_stop();
            xlog_exit();
            if (ch == 'h')
                return (0);
            else
                return (1);
        }
    }
    argc -= optind;
    argv += optind;

    int rval = 0;
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	rval  = test_main<IPv4>();
	rval |= test_main<IPv6>();
    } catch (...) {
        // Internal error
        xorp_print_standard_exceptions();
        rval = 2;
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return rval;
}
