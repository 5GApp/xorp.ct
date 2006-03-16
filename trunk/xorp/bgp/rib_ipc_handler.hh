// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/bgp/rib_ipc_handler.hh,v 1.40 2006/02/07 22:16:56 bms Exp $

#ifndef __BGP_RIB_IPC_HANDLER_HH__
#define __BGP_RIB_IPC_HANDLER_HH__

#include <deque>

#include "peer_handler.hh"
#include "plumbing.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/timer.hh"
#include "libxipc/xrl_std_router.hh"

#include "policy/backend/policytags.hh"

class RibIpcHandler;
class EventLoop;

template <class A>
class XrlQueue {
public:
    XrlQueue(RibIpcHandler &rib_ipc_handler, XrlStdRouter &xrl_router,
	     BGPMain &_bgp);

    void queue_add_route(string ribname, bool ibgp, Safi, const IPNet<A>& net,
			 const A& nexthop, const PolicyTags& policytags);

    void queue_delete_route(string ribname, bool ibgp, Safi,
			    const IPNet<A>& net);

    bool busy();
private:
    static const size_t WINDOW = 100;	// Maximum number of XRLs
					// allowed in flight.

    RibIpcHandler &_rib_ipc_handler;
    XrlStdRouter &_xrl_router;
    BGPMain &_bgp;

    struct Queued {
	bool add;
	string ribname;
	bool ibgp;
	Safi safi;
	IPNet<A> net;
	A nexthop;
	string comment;
	PolicyTags policytags;
    };

    deque <Queued> _xrl_queue;
    size_t _flying; //XRLs currently in flight

    /**
     * Maximum number in flight
     */
    inline bool maximum_number_inflight() const {
	return _flying >= WINDOW;
    }

    /**
     * Start the transmission of XRLs to tbe RIB.
     */
    void start();

    /**
     * The specialised method called by sendit to deal with IPv4/IPv6.
     *
     * @param q the queued command.
     * @param bgp "ibg"p or "ebgp".
     * @return True if the add/delete was queued.
     */
    bool sendit_spec(Queued& q, const char *bgp);

    inline EventLoop& eventloop() const;

    void route_command_done(const XrlError& error, const string comment);
};

/*
 * RibIpcHandler's job is to convert to and from XRLs, and to handle the
 * XRL state machine for talking to the RIB process 
 */

class RibIpcHandler : public PeerHandler {
public:
    RibIpcHandler(XrlStdRouter& xrl_router, BGPMain& bgp);

    ~RibIpcHandler();

    /**
    * Set the rib's name, allows for having a dummy rib or not having
    * a RIB at all.
    */ 
    bool register_ribname(const string& r);
    int start_packet();
    /* add_route and delete_route are called to propagate a route *to*
       the RIB. */
    int add_route(const SubnetRoute<IPv4> &rt, bool ibgp, Safi safi);
    int add_route(const SubnetRoute<IPv6> &rt, bool ibgp, Safi safi);
    int replace_route(const SubnetRoute<IPv4> &old_rt, bool old_ibgp, 
		      const SubnetRoute<IPv4> &new_rt, bool new_ibgp, 
		      Safi safi);
    int replace_route(const SubnetRoute<IPv6> &old_rt, bool old_ibgp, 
		      const SubnetRoute<IPv6> &new_rt, bool old_ibgp, 
		      Safi safi);
    int delete_route(const SubnetRoute<IPv4> &rt, bool ibgp, Safi safi);
    int delete_route(const SubnetRoute<IPv6> &rt, bool ibgp, Safi safi);
    void rib_command_done(const XrlError& error, const char *comment);
    PeerOutputState push_packet();

    void set_plumbing(BGPPlumbing *plumbing_unicast,
		      BGPPlumbing *plumbing_multicast) {
	_plumbing_unicast = plumbing_unicast;
	_plumbing_multicast = plumbing_multicast;
    }

    /**
     * @return true if routes are being sent to the RIB.
     */
    bool busy() {
	return _v4_queue.busy() || _v6_queue.busy();
    }

    /**
     * Originate an IPv4 route
     *
     * @param origin of the path information
     * @param aspath
     * @param nlri subnet to announce
     * @param next_hop to forward to
     * @param unicast if true install in unicast routing table
     * @param multicast if true install in multicast routing table
     * @param policytags policy-tags associated with the route
     *
     * @return true on success
     */
    bool originate_route(const OriginType origin,
			 const AsPath& aspath,
			 const IPv4Net& nlri,
			 const IPv4& next_hop,
			 const bool& unicast,
			 const bool& multicast,
			 const PolicyTags& policytags);

    /**
     * Originate an IPv6 route
     *
     * @param origin of the path information
     * @param aspath
     * @param nlri subnet to announce
     * @param next_hop to forward to
     * @param unicast if true install in unicast routing table
     * @param multicast if true install in multicast routing table
     * @param policytags policy-tags associated with the route
     *
     * @return true on success
     */
    bool originate_route(const OriginType origin,
			 const AsPath& aspath,
			 const IPv6Net& nlri,
			 const IPv6& next_hop,
			 const bool& unicast,
			 const bool& multicast,
			 const PolicyTags& policytags);

    /**
     * Withdraw an IPv4 route
     *
     * @param nlri subnet to withdraw
     * @param unicast if true withdraw from unicast routing table
     * @param multicast if true withdraw from multicast routing table
     *
     * @return true on success
     */
    bool withdraw_route(const IPv4Net&	nlri,
			const bool& unicast,
			const bool& multicast);

    /**
     * Withdraw an IPv6 route
     *
     * @return true on success
     */
    bool withdraw_route(const IPv6Net&	nlri,
			const bool& unicast,
			const bool& multicast);

    
    virtual const uint32_t get_unique_id() const { return _fake_unique_id; }

    //fake a zero IP address so the RIB IPC handler gets listed first
    //in the Fanout Table.
    const IPv4& id() const		{ return _fake_id; }

    virtual PeerType get_peer_type() const {
	return PEER_TYPE_INTERNAL;
    }                                                                           
    /**
     * This is the handler that deals with originating routes.
     *
     * @return true
     */
    virtual bool originate_route_handler() const {return true;}

    /**
     * @return the main eventloop
     */
    virtual EventLoop& eventloop() const { return _xrl_router.eventloop();}
private:
    bool unregister_rib(string ribname);

    string _ribname;
    XrlStdRouter& _xrl_router;

    XrlQueue<IPv4> _v4_queue;
    XrlQueue<IPv6> _v6_queue;
    const uint32_t _fake_unique_id;
    IPv4 _fake_id;
};

#endif // __BGP_RIB_IPC_HANDLER_HH__
