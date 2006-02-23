// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/ospf.cc,v 1.64 2006/02/02 01:20:51 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <map>
#include <list>
#include <set>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "ospf.hh"

template <typename A>
Ospf<A>::Ospf(OspfTypes::Version version, EventLoop& eventloop, IO<A>* io)
    : _version(version), _eventloop(eventloop),
      _io(io), _reason("Waiting for IO"), _process_status(PROC_STARTUP),
      _lsa_decoder(version), _peer_manager(*this), _routing_table(*this)
{
    // Register the LSAs and packets with the associated decoder.
    initialise_lsa_decoder(version, _lsa_decoder);
    initialise_packet_decoder(version, _packet_decoder, _lsa_decoder);

    // Now that all the packet decoders are in place register for
    // receiving packets.
    _io->register_receive(callback(this,&Ospf<A>::receive));

    // The peer manager will solicit packets from the various interfaces.
}

/**
 * All packets for OSPF are received through this interface. All good
 * packets are sent to the peer manager which verifies that the packet
 * is expected and authenticates the packet if necessary. The peer
 * manager can choose to accept the packet in which case it becomes
 * the owner. If the packet is rejected this routine will delete the
 * packet.
 */
template <typename A>
void 
Ospf<A>::receive(const string& interface, const string& vif,
		 A dst, A src, uint8_t* data, uint32_t len)
{
    debug_msg("Interface %s Vif %s dst %s src %s data %p len %u\n",
	      interface.c_str(), vif.c_str(),
	      dst.str().c_str(), src.str().c_str(),
	      data, len);

    Packet *packet;
    try {
	packet = _packet_decoder.decode(data, len);
    } catch(BadPacket& e) {
	XLOG_ERROR("%s", cstring(e));
	return;
    }

    debug_msg("%s\n", packet->str().c_str());
    // We have a packet and its good.

    bool packet_accepted = false;
    try {
	packet_accepted = _peer_manager.receive(interface, vif, dst, src,
						packet);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
    }

    if (!packet_accepted)
	delete packet;
}

template <typename A>
bool
Ospf<A>::enable_interface_vif(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    if (string(VLINK) == interface)
	return true;

    return _io->enable_interface_vif(interface, vif);
}

template <typename A>
bool
Ospf<A>::disable_interface_vif(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    if (string(VLINK) == interface)
	return true;

    return _io->disable_interface_vif(interface, vif);
}

template <typename A>
bool
Ospf<A>::enabled(const string& interface, const string& vif, A address)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    return _io->is_address_enabled(interface, vif, address);
}

template <typename A>
bool
Ospf<A>::get_prefix_length(const string& interface, const string& vif,
			   A address, uint16_t& prefix_length)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    if (string(VLINK) == interface) {
	prefix_length = 0;
	return true;
    }

    prefix_length = _io->get_prefix_length(interface, vif, address);
    return 0 == prefix_length ? false : true;
}

template <typename A>
uint32_t
Ospf<A>::get_mtu(const string& interface)
{
    debug_msg("Interface %s\n", interface.c_str());

    if (string(VLINK) == interface)
	return VLINK_MTU;

    return _io->get_mtu(interface);
}

template <typename A>
bool
Ospf<A>::join_multicast_group(const string& interface, const string& vif,
			      A mcast)
{
    debug_msg("Interface %s Vif %s mcast %s\n", interface.c_str(),
	      vif.c_str(), cstring(mcast));

    return _io->join_multicast_group(interface, vif, mcast);
}

template <typename A>
bool
Ospf<A>::leave_multicast_group(const string& interface, const string& vif,
			      A mcast)
{
    debug_msg("Interface %s Vif %s mcast %s\n", interface.c_str(),
	      vif.c_str(), cstring(mcast));

    return _io->leave_multicast_group(interface, vif, mcast);
}

template <typename A>
bool
Ospf<A>::transmit(const string& interface, const string& vif,
		  A dst, A src,
		  uint8_t* data, uint32_t len)
{
    debug_msg("Interface %s Vif %s data %p len %u\n",
	      interface.c_str(), vif.c_str(), data, len);
#ifdef	DEBUG_LOGGING
    try {
	// Decode the packet in order to pretty print it.
	Packet *packet = _packet_decoder.decode(data, len);
	debug_msg("Transmit: %s\n", cstring(*packet));
	delete packet;
    } catch(BadPacket& e) {
	debug_msg("Unable to decode packet\n");
    }
#endif

    return _io->send(interface, vif, dst, src, data, len);
}

template <typename A>
bool
Ospf<A>::set_interface_id(const string& interface, const string& vif,
			  OspfTypes::AreaID area,
			  uint32_t interface_id)
{
    try {
	_peer_manager.set_interface_id(_peer_manager.get_peerid(interface,vif),
				       area, interface_id);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_hello_interval(const string& interface, const string& vif,
			    OspfTypes::AreaID area,
			    uint16_t hello_interval)
{
    try {
	_peer_manager.set_hello_interval(_peer_manager.
					 get_peerid(interface, vif),
					 area, hello_interval);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

#if	0
template <typename A>
bool 
Ospf<A>::set_options(const string& interface, const string& vif,
		     OspfTypes::AreaID area,
		     uint32_t options)
{
    try {
	_peer_manager.set_options(_peer_manager.get_peerid(interface, vif),
				  area, options);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}
#endif

template <typename A>
bool
Ospf<A>::create_virtual_link(OspfTypes::RouterID rid)
{
    try {
	_peer_manager.create_virtual_link(rid);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::delete_virtual_link(OspfTypes::RouterID rid)
{
    try {
	_peer_manager.delete_virtual_link(rid);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::transit_area_virtual_link(OspfTypes::RouterID rid,
				   OspfTypes::AreaID transit_area)
{
    try {
	_peer_manager.transit_area_virtual_link(rid, transit_area);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}
    
template <typename A>
bool
Ospf<A>::set_router_priority(const string& interface, const string& vif,
			     OspfTypes::AreaID area,
			     uint8_t priority)
{
    try {
	_peer_manager.set_router_priority(_peer_manager.
					  get_peerid(interface, vif),
					  area, priority);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_router_dead_interval(const string& interface, const string& vif,
			 OspfTypes::AreaID area,
			 uint32_t router_dead_interval)
{
    try {
	_peer_manager.set_router_dead_interval(_peer_manager.
					       get_peerid(interface,vif),
					       area, router_dead_interval);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_interface_cost(const string& interface, const string& vif,
			    OspfTypes::AreaID area,
			    uint16_t interface_cost)
{
    try {
	_peer_manager.set_interface_cost(_peer_manager.
					 get_peerid(interface,vif),
					 area, interface_cost);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_retransmit_interval(const string& interface, const string& vif,
				 OspfTypes::AreaID area,
				 uint16_t retransmit_interval)
{
    if (0 == retransmit_interval) {
	XLOG_ERROR("Zero is not a legal value for RxmtInterval");
	return false;
    }

    try {
	_peer_manager.set_retransmit_interval(_peer_manager.
					      get_peerid(interface,vif),
					      area, retransmit_interval);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_inftransdelay(const string& interface, const string& vif,
			   OspfTypes::AreaID area,
			   uint16_t inftransdelay)
{
    if (0 == inftransdelay) {
	XLOG_ERROR("Zero is not a legal value for inftransdelay");
	return false;
    }

    try {
	_peer_manager.set_inftransdelay(_peer_manager.
					get_peerid(interface,vif),
					area, inftransdelay);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_simple_authentication_key(const string&		interface,
				       const string&		vif,
				       OspfTypes::AreaID	area,
				       const string&		password,
				       string&			error_msg)
{
    PeerID peerid;

    try {
	peerid = _peer_manager.get_peerid(interface, vif);
    } catch(BadPeer& e) {
	error_msg = e.str();
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    } 

    if (_peer_manager.set_simple_authentication_key(peerid, area,
						    password, error_msg)
	!= true) {
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    }

    return true;
}

template <typename A>
bool
Ospf<A>::delete_simple_authentication_key(const string&		interface,
					  const string&		vif,
					  OspfTypes::AreaID	area,
					  string&		error_msg)
{
    PeerID peerid;

    try {
	peerid = _peer_manager.get_peerid(interface, vif);
    } catch(BadPeer& e) {
	error_msg = e.str();
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    } 

    if (_peer_manager.delete_simple_authentication_key(peerid, area, error_msg)
	!= true) {
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    }

    return true;
}

template <typename A>
bool
Ospf<A>::set_md5_authentication_key(const string&		interface,
				    const string&		vif,
				    OspfTypes::AreaID		area,
				    uint8_t			key_id,
				    const string&		password,
				    uint32_t			start_secs,
				    uint32_t			end_secs,
				    string&			error_msg)

{
    PeerID peerid;

    try {
	peerid = _peer_manager.get_peerid(interface, vif);
    } catch(BadPeer& e) {
	error_msg = e.str();
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    } 

    if (_peer_manager.set_md5_authentication_key(peerid, area, key_id,
						 password, start_secs,
						 end_secs, error_msg)
	!= true) {
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    }

    return true;
}

template <typename A>
bool
Ospf<A>::delete_md5_authentication_key(const string&		interface,
				       const string&		vif,
				       OspfTypes::AreaID	area,
				       uint8_t			key_id,
				       string&			error_msg)
{
    PeerID peerid;

    try {
	peerid = _peer_manager.get_peerid(interface, vif);
    } catch(BadPeer& e) {
	error_msg = e.str();
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    } 

    if (_peer_manager.delete_md5_authentication_key(peerid, area, key_id,
						    error_msg)
	!= true) {
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    }

    return true;
}

template <typename A>
bool
Ospf<A>::set_passive(const string& interface, const string& vif,
		     OspfTypes::AreaID area, bool passive)
{
    try {
	_peer_manager.set_passive(_peer_manager.
				  get_peerid(interface,vif),
				  area, passive);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool 
Ospf<A>::set_ip_router_alert(bool alert)
{
    return _io->set_ip_router_alert(alert);
}

template <typename A>
bool
Ospf<A>::area_range_add(OspfTypes::AreaID area, IPNet<A> net, bool advertise)
{
    debug_msg("Area %s Net %s advertise %s\n", pr_id(area).c_str(),
	      cstring(net), pb(advertise));

    return _peer_manager.area_range_add(area, net, advertise);
}

template <typename A>
bool
Ospf<A>::area_range_delete(OspfTypes::AreaID area, IPNet<A> net)
{
    debug_msg("Area %s Net %s\n", pr_id(area).c_str(), cstring(net));

    return _peer_manager.area_range_delete(area, net);
}

template <typename A>
bool
Ospf<A>::area_range_change_state(OspfTypes::AreaID area, IPNet<A> net,
				 bool advertise)
{
    debug_msg("Area %s Net %s advertise %s\n", pr_id(area).c_str(),
	      cstring(net), pb(advertise));

    return _peer_manager.area_range_change_state(area, net, advertise);
}

template <typename A>
bool
Ospf<A>::get_lsa(const OspfTypes::AreaID area, const uint32_t index,
		 bool& valid, bool& toohigh, bool& self, vector<uint8_t>& lsa)
{
    debug_msg("Area %s index %u\n", pr_id(area).c_str(), index);

    return _peer_manager.get_lsa(area, index, valid, toohigh, self, lsa);
}

template <typename A>
bool
Ospf<A>::get_area_list(list<OspfTypes::AreaID>& areas) const
{
    debug_msg("\n");

    return _peer_manager.get_area_list(areas);
}

template <typename A>
bool
Ospf<A>::get_neighbour_list(list<OspfTypes::NeighbourID>& neighbours) const
{
    debug_msg("\n");

    return _peer_manager.get_neighbour_list(neighbours);
}

template <typename A>
bool
Ospf<A>::get_neighbour_info(OspfTypes::NeighbourID nid,
			    NeighbourInfo& ninfo) const
{
    return _peer_manager.get_neighbour_info(nid, ninfo);
}

template <typename A>
bool
Ospf<A>::add_route(IPNet<A> net, A nexthop, uint32_t metric, bool equal,
		   bool discard, const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	      cstring(net), cstring(nexthop), metric, pb(equal),
	      pb(discard), cstring(policytags));

    XLOG_TRACE(trace()._routes,
	       "Add route "
	       "Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	      cstring(net), cstring(nexthop), metric, pb(equal),
	      pb(discard), cstring(policytags));

    return _io->add_route(net, nexthop, metric, equal, discard, policytags);
}

template <typename A>
bool
Ospf<A>::replace_route(IPNet<A> net, A nexthop, uint32_t metric, bool equal,
			bool discard, const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	      cstring(net), cstring(nexthop), metric, pb(equal),
	      pb(discard), cstring(policytags));

    XLOG_TRACE(trace()._routes,
	       "Replace route "
	       "Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	      cstring(net), cstring(nexthop), metric, pb(equal),
	      pb(discard), cstring(policytags));

    return _io->replace_route(net, nexthop, metric, equal, discard,
			      policytags);
}

template <typename A>
bool
Ospf<A>::delete_route(IPNet<A> net)
{
    debug_msg("Net %s\n", cstring(net));

    XLOG_TRACE(trace()._routes, "Delete route Net %s\n", cstring(net));

    return _io->delete_route(net);
}

template <typename A>
void
Ospf<A>::configure_filter(const uint32_t& filter, const string& conf)
{
    _policy_filters.configure(filter,conf);
}

template <typename A>
void
Ospf<A>::reset_filter(const uint32_t& filter)
{
    _policy_filters.reset(filter);
}

template <typename A>
void
Ospf<A>::push_routes()
{
    XLOG_WARNING("TBD - policy route pushing");
}

template <typename A>
bool 
Ospf<A>::originate_route(const IPNet<A>& net,
			 const A& nexthop,
			 const uint32_t& metric,
			 const PolicyTags& policytags)
{
    return _peer_manager.external_announce(net, nexthop, metric, policytags);
}

template <typename A>
bool 
Ospf<A>::withdraw_route(const IPNet<A>& net)
{
    return _peer_manager.external_withdraw(net);
}

template <typename A>
void
Ospf<A>::set_router_id(OspfTypes::RouterID id)
{
    _peer_manager.router_id_changing();
    _router_id = id;
}

template class Ospf<IPv4>;
template class Ospf<IPv6>;
