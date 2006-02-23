
// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

// $XORP: xorp/bgp/route_table_filter.hh,v 1.23 2005/11/28 04:55:01 atanu Exp $

#ifndef __BGP_ROUTE_TABLE_FILTER_HH__
#define __BGP_ROUTE_TABLE_FILTER_HH__

#include <set>
#include <map>
#include "route_table_base.hh"
#include "next_hop_resolver.hh"
#include "peer_data.hh"

/**
 * @short Base class for a single filter within FilterTable's filter bank.
 *
 * FilterTable implements a bank of filters for modifying or dropping
 * routes. Each filter within such a filter bank is a BGPRouteFilter.
 * BGPRouteFilter is a generic filter, and so needs specializing in a
 * subclass to actually implement a filter.
 */

template<class A>
class BGPRouteFilter {
public:
    BGPRouteFilter() {};
    virtual ~BGPRouteFilter() {}

    /* "modified" is needed because we need to know whether we should
       free the rtmsg or not if we modify the route. If this
       filterbank has modified it, and then modifies it again, then we
       should free the rtmsg.  Otherwise it's the responsibility of
       whoever created the rtmsg*/
    virtual const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const = 0;
protected:
    void propagate_flags(const InternalMessage<A> *rtmsg,
			 InternalMessage<A> *new_rtmsg) const;

    void propagate_flags(const SubnetRoute<A>& route,
			 SubnetRoute<A>& new_route) const;

    virtual void drop_message(const InternalMessage<A> *rtmsg, 
			      bool &modified) const ;
private:
};


/**
 * @short filters out aggregate routes depending whether on IBGP or EBGP
 * outbound branch.
 */

template<class A>
class AggregationFilter : public BGPRouteFilter<A> {
public:
    AggregationFilter(bool is_ibgp);
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const ;
private:
    bool _is_ibgp;
};


/**
 * @short BGPRouteFilter that drops routes that have a particular AS
 * in their AS path.
 *
 * SimpleASFilter is a BGPRouteFilter that drops routes that have a
 * particular AS in their AS path.  This is typically used in loop
 * prevention, where an inbound filter from an EBGP peer drops routes
 * that contain our own AS number
 * 
 * This works on both regular EBGP and CONFEDERATION EBGP sessions 
 */

template<class A>
class SimpleASFilter : public BGPRouteFilter<A> {
public:
    SimpleASFilter(const AsNum &as_num);
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const ;
private:
    AsNum _as_num;
};


/**
 * @short BGPRouteFilter that drops routes with this routers
 * ORIGINATOR_ID or CLUSTER_ID.
 *
 * RRInputFilter is a BGPRouteFilter that drops with this routers
 * ORIGINATOR_ID or CLUSTER_ID. An inbound filter that is configured
 * on IBGP peerings when this router is a route reflector.
 * 
 */

template<class A>
class RRInputFilter : public BGPRouteFilter<A> {
public:
    RRInputFilter(IPv4 bgp_id, IPv4 cluster_id);
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const ;
private:
    IPv4 _bgp_id;
    IPv4 _cluster_id;
};


/**
 * @short BGPRouteFilter that prepends an AS to the AS path.
 *
 * ASPrependFilter is a BGPRouteFilter that prepends an AS to the AS
 * path.  This is typically used when sending a route to an EBGP peer
 * to add our own AS number to the path.
 * 
 * This works on both regular EBGP and CONFEDERATION EBGP sessions 
 */

template<class A>
class ASPrependFilter : public BGPRouteFilter<A> {
public:
    ASPrependFilter(const AsNum &as_num, bool is_confederation_peer);
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const;
private:
    AsNum _as_num;
    bool _is_confederation_peer; 
};


/**
 * @short BGPRouteFilter that changes the nexthop attribute in a route.
 *
 * NexthopRewriteFilter is a BGPRouteFilter that changes the nexthop
 * attribute in a route passing though the filter.  A typicaly use is
 * when passing a route to an EBGP peer, we change the nexthop
 * attribute to be our own IP address on the appropriate interface to
 * that peer.
 */

template<class A>
class NexthopRewriteFilter : public BGPRouteFilter<A> {
public:
    NexthopRewriteFilter(const A &local_nexthop);
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const ;
private:
    A _local_nexthop;
};


/**
 * @short BGPRouteFilter that drops routes that came to us from an IBGP peer.
 *
 * IBGPLoopFilter is a BGPRouteFilter that drops routes that
 * came to us from an IBGP peer.  Typically it is used in a outgoing
 * filter on a branch to another IBGP peer, and prevents routes coming
 * from one IBGP peer from being forwarded to another IBGP peer.
 */

template<class A>
class IBGPLoopFilter : public BGPRouteFilter<A> {
public:
    IBGPLoopFilter();
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const ;
private:
};


/**
 * @short BGPRouteFilter that drops or reflects routes from an IBGP
 * peer. Add the originator ID and the cluster ID.
 *
 * RRIBGPLoopFilter is a BGPRouteFilter that is a replacement for the
 * IBGLoopFilter when the router is a route reflector.
 * Incoming route came from:
 * 	E-BGP peer, route is passed.
 *      I-BGP peer, route is passed if this is a route reflector client.
 *      I-BGP client peer, route is passed to all types of client.
 */

template<class A>
class RRIBGPLoopFilter : public BGPRouteFilter<A> {
public:
    RRIBGPLoopFilter(bool rr_client, IPv4 bgp_id, IPv4 cluster_id);
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const ;
private:
    bool _rr_client;
    IPv4 _bgp_id;
    IPv4 _cluster_id;
};


/**
 * @short BGPRouteFilter that inserts a LocalPref attribute.
 *
 * LocalPrefInsertionFilter is a BGPRouteFilter that inserts a LocalPref
 * attribute into routes passing though the filter.  This is typically
 * used in an incoming filter bank from an EBGP peer to add LocalPref.
 */

template<class A>
class LocalPrefInsertionFilter : public BGPRouteFilter<A> {
public:
    LocalPrefInsertionFilter(uint32_t default_local_pref);
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
    uint32_t _default_local_pref;
};


/**
 * @short BGPRouteFilter that deletes a LocalPref attribute.
 *
 * LocalPrefRemovalFilter is a BGPRouteFilter that deletes the
 * LocalPref attribute (if present) from routes passing though the
 * filter.  This is typically used in an outgoing filter bank to an
 * EBGP peer.
 */

template<class A>
class LocalPrefRemovalFilter : public BGPRouteFilter<A> {
public:
    LocalPrefRemovalFilter();
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
};


/**
 * @short BGPRouteFilter that inserts a MED attribute.
 *
 * MEDInsertionFilter is a BGPRouteFilter that inserts a MED attribute
 * into routes passing though the filter.  This is typically used in
 * an outgoing filter bank to an EBGP peer.
 */

template<class A>
class MEDInsertionFilter : public BGPRouteFilter<A> {
public:
    MEDInsertionFilter(NextHopResolver<A>& next_hop_resolver);
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
    NextHopResolver<A>& _next_hop_resolver;
};

/**
 * @short BGPRouteFilter that removes a MED attribute.
 *
 * MEDInsertionFilter is a BGPRouteFilter that removes a MED attribute
 * from routes passing though the filter.  This is typically used in
 * an outgoing filter bank to an EBGP peer, before we add our own MED.
 */

template<class A>
class MEDRemovalFilter : public BGPRouteFilter<A> {
public:
    MEDRemovalFilter();
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
};

/**
 * @short BGPRouteFilter that processes well-known communities.
 *
 * KnownCommunityFilter is a BGPRouteFilter that drops routes with
 * certain well-known communities.  See RFC 1997 and the IANA registry.
 */

template<class A>
class KnownCommunityFilter : public BGPRouteFilter<A> {
public:
    KnownCommunityFilter(PeerType peer_type);
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
    PeerType _peer_type;
};

/**
 * @short BGPRouteFilter that processes unknown attributes.
 *
 * UnknownFilter is a BGPRouteFilter that processes unknown attributes
 * according to their flags.  It is typically used in an outgoing
 * filter bank to avoid incorrectly passing unknown non-transitive
 * attributes on to a peer, and to set the partial but on unknown
 * transitive attributes.
 */

template<class A>
class UnknownFilter : public BGPRouteFilter<A> {
public:
    UnknownFilter();
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
};


/**
 * Perform any filter operations that are required for routes that we
 * originate.
 * 
 * Currently this only involves prepending our AS number on IBGP
 * peers. We assume that on EBGP peers the AS number has already been
 * prepended.
 *
 */

template<class A>
class OriginateRouteFilter : public BGPRouteFilter<A> {
public:
    OriginateRouteFilter(const AsNum &as_num, PeerType peer_type);
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
    AsNum _as_num;
    PeerType _peer_type;
};

/**
 * @short specific version of a static route filter
 *
 * When a peering is reconfigured, we need to change the filter, but
 * still maintain consistency for downstream tables.  We keep the old
 * filter around until there are no more routes that need it, and only
 * then delete it.  FilterVersion holds a specific version of a static
 * route filter bank.
 */

template<class A>
class FilterVersion {
public:
    FilterVersion(NextHopResolver<A>& next_hop_resolver);
    ~FilterVersion();
    void set_genid(uint32_t genid) {_genid = genid;}
    int add_aggregation_filter(bool is_ibgp);
    int add_simple_AS_filter(const AsNum &asn);
    int add_route_reflector_input_filter(IPv4 bgp_id, IPv4 cluster_id);
    int add_AS_prepend_filter(const AsNum &asn, bool is_confederation_peer);
    int add_nexthop_rewrite_filter(const A& nexthop);
    int add_ibgp_loop_filter();
    int add_route_reflector_ibgp_loop_filter(bool client,
					     IPv4 bgp_id,
					     IPv4 cluster_id);
    int add_localpref_insertion_filter(uint32_t default_local_pref);
    int add_localpref_removal_filter();
    int add_med_insertion_filter();
    int add_med_removal_filter();
    int add_known_community_filter(PeerType peer_type);
    int add_unknown_filter();
    int add_originate_route_filter(const AsNum &asn, PeerType peer_type);
    const InternalMessage<A> *
        apply_filters(const InternalMessage<A> *rtmsg, int ref_change);
    int ref_count() const {return _ref_count;}
    uint32_t genid() const {return _genid;}
    bool used() const {return _used;}
private:
    uint32_t _genid;
    bool _used;
    list <BGPRouteFilter<A> *> _filters;
    int _ref_count;
    NextHopResolver<A>& _next_hop_resolver;
};

/**
 * @short specialized BGPRouteTable implementing a filter bank to
 * modify or drop routes.
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
 * FilterTable is a BGPRouteTable that can hold banks of route
 * filters.  Normally FilterTable propagates add_route,
 * delete_route and the response to lookup_route directly through from
 * the parent to the child.  A route filter can cause these to not be
 * be propagated, or can modify the attributes of the route in the
 * message.
 *
 * Typically there are two FilterTables for each peer, one in the
 * input branch from that peer, and one in the output branch to that
 * peer.  A FilterTable does not store the routes it modifies.  Thus
 * is is normally followed by a CacheTable which stores those routes
 * for later use.
 */

template<class A>
class FilterTable : public BGPRouteTable<A>  {
public:
    FilterTable(string tablename,
		Safi safi,
		BGPRouteTable<A> *parent, 
		NextHopResolver<A>& next_hop_resolver);
    ~FilterTable();
    void reconfigure_filter();

    int add_route(const InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(const InternalMessage<A> &old_rtmsg,
		      const InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(const InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int route_dump(const InternalMessage<A> &rtmsg,
		   BGPRouteTable<A> *caller,
		   const PeerHandler *dump_peer);
    int push(BGPRouteTable<A> *caller);
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid) const;
    void route_used(const SubnetRoute<A>* route, bool in_use);

    RouteTableType type() const {return FILTER_TABLE;}
    string str() const;

    /* mechanisms to implement flow control in the output plumbing */
    bool get_next_message(BGPRouteTable<A> *next_table);

    int add_aggregation_filter(bool is_ibgp);
    int add_simple_AS_filter(const AsNum &asn);
    int add_route_reflector_input_filter(IPv4 bgp_id, IPv4 cluster_id);
    int add_AS_prepend_filter(const AsNum &asn, bool is_confederation_peer);
    int add_nexthop_rewrite_filter(const A& nexthop);
    int add_ibgp_loop_filter();
    int add_route_reflector_ibgp_loop_filter(bool client,
					     IPv4 bgp_id,
					     IPv4 cluster_id);
    int add_localpref_insertion_filter(uint32_t default_local_pref);
    int add_localpref_removal_filter();
    int add_med_insertion_filter();
    int add_med_removal_filter();
    int add_known_community_filter(PeerType peer_type);
    int add_unknown_filter();
    int add_originate_route_filter(const AsNum &asn, PeerType peer_type);
    void do_versioning() {_do_versioning = true;}

private:
    const InternalMessage<A> *
        apply_filters(const InternalMessage<A> *rtmsg, int ref_change);
    const InternalMessage<A> *
        apply_filters(const InternalMessage<A> *rtmsg) const;
    map <uint32_t, FilterVersion<A>* > _filter_versions;
    set <uint32_t> _deleted_filters;  /* kept as a sanity check */
    FilterVersion<A>* _current_filter;
    NextHopResolver<A>& _next_hop_resolver;
    bool _do_versioning;
};

#endif // __BGP_ROUTE_TABLE_FILTER_HH__
