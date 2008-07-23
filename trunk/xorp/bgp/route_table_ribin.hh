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

// $XORP: xorp/bgp/route_table_ribin.hh,v 1.28 2008/01/04 03:15:26 pavlin Exp $

#ifndef __BGP_ROUTE_TABLE_RIBIN_HH__
#define __BGP_ROUTE_TABLE_RIBIN_HH__


#include <map>
#include "libxorp/eventloop.hh"
#include "route_table_base.hh"
#include "crash_dump.hh"
#include "bgp_trie.hh"

class EventLoop;

/**
 * @short Specialized BGPRouteTable that stores routes from a BGP peer.
 *
 * The XORP BGP is internally implemented as a set of pipelines
 * consisting of a series of BGPRouteTables.  Each pipeline receives
 * routes from a BGP peer, stores them, and applies filters to them to
 * modify the routes.  Then the pipelines converge on a single
 * decision process, which decides which route wins amongst possible
 * alternative routes.  After decision, the winning routes fanout
 * again along a set of pipelines, again being filtered, before being
 * transmitted to peers.
 *
 * RibInTable is the first stage in such a pipeline.  It receives
 * routes from a single BGP peer, and stores them.  Changes are passed
 * down the pipeline as they occur, and route lookups from downstream
 * are answered by the RibInTable.
 */ 

template<class A>
class RibInTable : public BGPRouteTable<A>, CrashDumper {
public:
    RibInTable(string tablename, Safi safi, const PeerHandler *peer);
    ~RibInTable();
    /**
     * Remove all the stored routes. Used to flush static routes only.
     */
    void flush();
    int add_route(const InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);

    void ribin_peering_went_down();
    void ribin_peering_came_up();

    /*replace_route doesn't make sense for a RibIn because it's the
      RibIn that decides whether this is an add or a replace*/
    int replace_route(const InternalMessage<A> & /*old_rtmsg*/,
		      const InternalMessage<A> & /*new_rtmsg*/,
		      BGPRouteTable<A> * /*caller*/ ) { abort(); return 0; }

    int delete_route(const InternalMessage<A> &rtmsg,
		     BGPRouteTable<A> *caller);

    int push(BGPRouteTable<A> *caller);
    int delete_add_routes();
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net, 
				       uint32_t& genid) const;
    void route_used(const SubnetRoute<A>* route, bool in_use);

    BGPRouteTable<A> *parent() { return NULL; }

    RouteTableType type() const { return RIB_IN_TABLE; }

    string str() const;

    bool get_next_message(BGPRouteTable<A> */*next_table*/) {
	abort();
	return false;
    }
    void set_peer_is_up() { _peer_is_up = true; }

    bool dump_next_route(DumpIterator<A>& dump_iter);

    /*igp_nexthop_changed is called when the IGP routing changes in
      such a way that IGP information that was being used by BGP for
      its decision process is affected.  We need to scan through the
      RIB, looking for routes that have this nexthop, and propagate
      them downstream again to see if the change makes a difference */
    void igp_nexthop_changed(const A& bgp_nexthop);

    int route_count() const {
	return _route_table->route_count();
    }

    BgpTrie<A>& trie() const {
	return *_route_table;
    }

    const PeerHandler* peer_handler() const {
	return _peer;
    }

    uint32_t genid() const {
	return _genid;
    }

    void crash_dump() const {
	CrashDumper::crash_dump();
    }
    string dump_state() const;

private:
    EventLoop& eventloop() const;

    BgpTrie<A>* _route_table;
    const PeerHandler *_peer;
    bool _peer_is_up;
    uint32_t _genid;
    uint32_t _table_version;

    // state and methods related to re-sending all the routes related
    // to a nexthop whose IGP information has changed.
    set <A> _changed_nexthops;
    bool _nexthop_push_active;
    A _current_changed_nexthop;
    typename BgpTrie<A>::PathmapType::const_iterator _current_chain;
    XorpTask _push_task;
    bool push_next_changed_nexthop();
    void deletion_nexthop_check(const SubnetRoute<A>* route);
    void next_chain();
    void stop_nexthop_push();
    // end of IGP nexthop handing stuff
};

#endif // __BGP_ROUTE_TABLE_RIBIN_HH__
