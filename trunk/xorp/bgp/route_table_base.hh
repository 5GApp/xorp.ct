// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

// $XORP: xorp/bgp/route_table_base.hh,v 1.15 2006/02/17 23:34:53 zec Exp $

#ifndef __BGP_ROUTE_TABLE_BASE_HH__
#define __BGP_ROUTE_TABLE_BASE_HH__

#include <iostream>
#include <string>

#include "libxorp/ipv4net.hh"
#include "libxorp/ipv4.hh"
#include "internal_message.hh"
//#include "dump_iterators.hh"

template<class A>
class DumpIterator;

enum RouteTableType {
    RIB_IN_TABLE = 1,
    RIB_OUT_TABLE = 2,
    CACHE_TABLE = 3,
    DECISION_TABLE = 4,
    FANOUT_TABLE = 5,
    FILTER_TABLE = 6,
    DELETION_TABLE = 7,
    DUMP_TABLE = 8,
    NHLOOKUP_TABLE = 9,
    DEBUG_TABLE = 10,
    POLICY_TABLE = 11,
    AGGREGATION_TABLE = 12,
    DAMPING_TABLE = 13
};

#define MAX_TABLE_TYPE 13

#define ADD_USED 1
#define ADD_UNUSED 2
#define ADD_FAILURE 3 
#define ADD_FILTERED 4 

/**
 * @short Base class for a stage in BGP's internal plumbing
 *
 * The XORP BGP is internally implemented as a set of pipelines.  Each
 * pipeline receives routes from a BGP peer, stores them, and applies
 * filters to them to modify the routes.  Then the pipelines converge
 * on a single decision process, which decides which route wins
 * amongst possible alternative routes.  After decision, the winning
 * routes fanout again along a set of pipelines, again being filtered,
 * before being transmitted to peers.  
 *
 * Each stage in these pipelines is a BGPRouteTable.  BGPRouteTable is
 * a virtual base class, so all the stages consist of specialized
 * RouteTable class instances.
 */

template<class A>
class BGPRouteTable {
public:
    BGPRouteTable(string tablename, Safi safi);
    virtual ~BGPRouteTable();
    virtual int add_route(const InternalMessage<A> &rtmsg, 
			  BGPRouteTable<A> *caller) = 0;
    virtual int replace_route(const InternalMessage<A> &old_rtmsg, 
			      const InternalMessage<A> &new_rtmsg, 
			      BGPRouteTable<A> *caller) = 0;
    virtual int delete_route(const InternalMessage<A> &rtmsg, 
			     BGPRouteTable<A> *caller) = 0;
    virtual int route_dump(const InternalMessage<A> &rtmsg, 
			   BGPRouteTable<A> *caller,
			   const PeerHandler *dump_peer);
    virtual int push(BGPRouteTable<A> *caller) = 0;

    virtual const 
    SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				 uint32_t& genid) const = 0;

    virtual void route_used(const SubnetRoute<A>* /*route*/, 
			    bool /*in_use*/){
	abort();
    }

    void set_next_table(BGPRouteTable<A>* table) {
	_next_table = table;
    }
    BGPRouteTable<A> *next_table() { return _next_table;}

    /* parent is only supposed to be called on single-parent tables*/
    virtual BGPRouteTable<A> *parent() { return _parent; }
    virtual void set_parent(BGPRouteTable<A> *parent) { _parent = parent; }

    virtual RouteTableType type() const = 0;
    const string& tablename() const {return _tablename;}
    virtual string str() const = 0;

    /* mechanisms to implement flow control in the output plumbing */
    virtual void wakeup();
    virtual bool get_next_message(BGPRouteTable *) {abort(); return false; }

    virtual bool dump_next_route(DumpIterator<A>& dump_iter);
    /**
     * Notification that the status of this next hop has changed.
     *
     * @param bgp_nexthop The next hop that has changed.
     */
    virtual void igp_nexthop_changed(const A& bgp_nexthop);

    virtual void peering_went_down(const PeerHandler *peer, uint32_t genid,
				   BGPRouteTable<A> *caller);
    virtual void peering_down_complete(const PeerHandler *peer, uint32_t genid,
				       BGPRouteTable<A> *caller);
    virtual void peering_came_up(const PeerHandler *peer, uint32_t genid,
				 BGPRouteTable<A> *caller);

    Safi safi() const {return _safi; }
protected:
    BGPRouteTable<A> *_next_table, *_parent;
    string _tablename;
    const Safi _safi;
};

#endif // __BGP_ROUTE_TABLE_BASE_HH__
