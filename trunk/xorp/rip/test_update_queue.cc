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



#include <set>

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
#include "update_queue.hh"

#include "test_utils.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_updates";
static const char *program_description  = "Test RIP update queue";
static const char *program_version_id   = "0.1";
static const char *program_date         = "July, 2003";
static const char *program_copyright    = "See file LICENSE";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";


// ----------------------------------------------------------------------------
// Spoof Port that supports just a single Peer
//

template <typename A>
class SpoofPort : public Port<A> {
public:
    SpoofPort(PortManagerBase<A>& pm, A addr) : Port<A>(pm)
    {
	this->_peers.push_back(new Peer<A>(*this, addr));
	verbose_log("Constructing SpoofPort instance\n");
    }
    ~SpoofPort()
    {
	verbose_log("Destructing SpoofPort instance\n");
	while (this->_peers.empty() == false) {
	    delete this->_peers.front();
	    this->_peers.pop_front();
	}
    }
};

// ----------------------------------------------------------------------------
// Type specific helpers

template <typename A>
struct DefaultPeer {
    static A get();
};

template <>
IPv4 DefaultPeer<IPv4>::get() { return IPv4("10.0.0.1"); }

template <>
IPv6 DefaultPeer<IPv6>::get() { return IPv6("10::1"); }

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
	this->_ports.push_back(new SpoofPort<A>(*this, DefaultPeer<A>::get()));
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



static const IfMgrIfTree ift_dummy = IfMgrIfTree();

template <typename A>
class UpdateQueueTester
{
public:
    UpdateQueueTester(EventLoop& e)
	: _e(e), _rip_system(_e), _pm(_rip_system, ift_dummy)
    {
	_pm.the_port()->constants().set_expiry_secs(3);
	_pm.the_port()->constants().set_deletion_secs(2);
    }

    ~UpdateQueueTester()
    {
	RouteDB<A>& rdb = _rip_system.route_db();
	rdb.flush_routes();
	UpdateQueue<A>& uq = rdb.update_queue();
	uq.flush();
    }

    int run_test()
    {
	string ifname, vifname;		// XXX: not set, because not needed
	const uint32_t n_routes = 20000;

	verbose_log("Running test for IPv%u\n",
		    XORP_UINT_CAST(A::ip_version()));

	RouteDB<A>& rdb = _rip_system.route_db();
	UpdateQueue<A>& uq = _rip_system.route_db().update_queue();

	_fast_reader = uq.create_reader();
	_slow_reader = uq.create_reader();

	if (uq.updates_queued() != 0) {
	    verbose_log("Have updates when none expected\n");
	    return 1;
	}

	verbose_log("Generating nets\n");
	set<IPNet<A> > nets;
	make_nets(nets, n_routes);

	verbose_log("Creating routes for nets\n");

	for_each(nets.begin(), nets.end(),
		 RouteInjector<A>(rdb, A::ZERO(), ifname, vifname, 5,
				  _pm.the_peer()));

	if (uq.updates_queued() != n_routes) {
	    verbose_log("%u updates queued, expected %u\n",
			XORP_UINT_CAST(uq.updates_queued()),
			XORP_UINT_CAST(n_routes));
	    return 1;
	}

	uint32_t n = 0;
	for (n = 0; n < n_routes; n++) {
	    if (uq.get(_fast_reader) == 0) {
		verbose_log("Ran out of updates at %u.\n", XORP_UINT_CAST(n));
		return 1;
	    }
	    uq.next(_fast_reader);
	}

	if (uq.get(_fast_reader) != 0) {
	    verbose_log("Got an extra update.\n");
	    return 1;
	}

	bool expire = false;
	XorpTimer t = _e.set_flag_after_ms(6000, &expire);
	while (t.scheduled()) {
	    _e.run();
	}

	verbose_log("%u updates queued, expected %u\n",
		    XORP_UINT_CAST(uq.updates_queued()),
		    XORP_UINT_CAST(n_routes));

	for (n = 0; n < n_routes; n++) {
	    if (uq.get(_fast_reader) == 0) {
		verbose_log("Ran out of updates at %u.\n", XORP_UINT_CAST(n));
		return 1;
	    }
	    uq.next(_fast_reader);
	}

	for (n = 0; n < 2 * n_routes; n++) {
	    if (uq.get(_slow_reader) == 0) {
		verbose_log("Ran out of updates at %u.\n", XORP_UINT_CAST(n));
		return 1;
	    }
	    uq.next(_slow_reader);
	}

	if (uq.get(_slow_reader) != 0) {
	    verbose_log("Got an extra update.\n");
	    return 1;
	}

	uq.destroy_reader(_fast_reader);
	uq.destroy_reader(_slow_reader);

	if (uq.updates_queued() != 0) {
	    verbose_log("Updates queued (%u) when no readers present\n",
			XORP_UINT_CAST(uq.updates_queued()));
	    return 1;
	}

	_slow_reader = uq.create_reader();
	_fast_reader = uq.create_reader();

	verbose_log("Creating routes for nets (again)\n");

	for (typename set<IPNet<A> >::const_iterator n = nets.begin();
	     n != nets.end(); ++n) {
	    if (rdb.update_route(*n, A::ZERO(), ifname, vifname, 5, 0,
				 _pm.the_peer(), PolicyTags(),
				 false) == false) {
		verbose_log("Failed to add route for %s\n",
			    n->str().c_str());
		return 1;
	    }
	}

	verbose_log("flushing and waiting for route updates\n");

	uq.ffwd(_fast_reader);
	uq.flush();

	if (uq.updates_queued() != 0) {
	    verbose_log("Flushed and route count != 0\n");
	    return 1;
	}

	if (uq.get(_fast_reader) != 0) {
	    verbose_log("Got an extra update.\n");
 	    return 1;
	}

	if (uq.get(_slow_reader) != 0) {
	    verbose_log("Got an extra update.\n");
	    return 1;
	}

	expire = false;
	t = _e.set_flag_after_ms(6000, &expire);
	while (t.scheduled()) {
	    _e.run();
	}

	for (n = 0; n < n_routes; n++) {
	    if (uq.get(_slow_reader) == 0) {
		verbose_log("Ran out of updates at %u.\n", XORP_UINT_CAST(n));
		return 1;
	    }
	    uq.next(_slow_reader);

	    if (uq.get(_fast_reader) == 0) {
		verbose_log("Ran out of updates at %u.\n", XORP_UINT_CAST(n));
		return 1;
	    }
	    uq.next(_fast_reader);
	}

	if (uq.get(_fast_reader) != 0) {
	    verbose_log("Got an extra update.\n");
	    return 1;
	}

	if (uq.get(_slow_reader) != 0) {
	    verbose_log("Got an extra update.\n");
	    return 1;
	}
	
	verbose_log("Success\n");
	return 0;
    }

protected:
    EventLoop&		_e;
    System<A>		_rip_system;
    SpoofPortManager<A> _pm;

    typename UpdateQueue<A>::ReadIterator _fast_reader;
    typename UpdateQueue<A>::ReadIterator _slow_reader;
};



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

/*
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
	EventLoop e;

	UpdateQueueTester<IPv4> uqt4(e);
	rval |= uqt4.run_test();

	UpdateQueueTester<IPv6> uqt6(e);
	rval |= uqt6.run_test();
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
