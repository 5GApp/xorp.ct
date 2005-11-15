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

#ident "$XORP: xorp/bgp/xrl_target.cc,v 1.46 2005/11/02 01:09:28 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME
#define PROFILE_UTILS_REQUIRED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bgp_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "libxipc/xrl_error.hh"
#include "xrl/interfaces/profile_client_xif.hh"

#include "bgp.hh"
#include "iptuple.hh"
#include "xrl_target.hh"
//#include "profile_vars.hh"

XrlBgpTarget::XrlBgpTarget(XrlRouter *r, BGPMain& bgp)
	: XrlBgpTargetBase(r),
	  _bgp(bgp),
	  _awaiting_config(true),
	  _awaiting_as(true),
	  _awaiting_bgp_id(true),
	  _done(false)
{
}

XrlCmdError
XrlBgpTarget::common_0_1_get_target_name(string& name)
{
    name = "bgp";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::common_0_1_get_version(string& version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::common_0_1_get_status(
    // Output values, 
    uint32_t& status,
    string&	reason)
{
    status = _bgp.status(reason);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::common_0_1_shutdown()
{
    if(!_awaiting_config) {
	_bgp.terminate();
    } else {
	_awaiting_config = false;
	_done = true;
    }
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_bgp_version(
				      // Output values, 
				      uint32_t&	version)
{
    version = 4;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_local_config(
				   // Input values, 
				   const uint32_t&	as, 
				   const IPv4&		id)
{
    debug_msg("as %u id %s\n", XORP_UINT_CAST(as), id.str().c_str());

    /*
    ** We may already be configured so don't allow a reconfiguration.
    */
    if(!_awaiting_config)
	return XrlCmdError::COMMAND_FAILED("Attempt to reconfigure BGP");

    _bgp.local_config(as, id);

    _awaiting_config = false;

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_set_local_as(
				   // Input values, 
				   const uint32_t&	as)
{
    debug_msg("as %u\n", XORP_UINT_CAST(as));

    /*
    ** We may already be configured so don't allow a reconfiguration.
    */
//     if(!_awaiting_as)
// 	return XrlCmdError::COMMAND_FAILED("Attempt to reconfigure BGP AS");

    _as = as;
    _awaiting_as = false;
    if(!_awaiting_as && !_awaiting_bgp_id) {
	_bgp.local_config(_as, _id);
	_awaiting_config = false;	
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_local_as(
				   // Output values, 
				   uint32_t& as) 
{
    if(_awaiting_as)
	return XrlCmdError::COMMAND_FAILED("BGP AS not yet configured");
    as = _as;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_set_bgp_id(
				 // Input values, 
				 const IPv4&	id)
{
    debug_msg("id %s\n", id.str().c_str());

    /*
    ** We may already be configured so don't allow a reconfiguration.
    */
//     if(!_awaiting_bgp_id)
// 	return XrlCmdError::COMMAND_FAILED("Attempt to reconfigure BGP ID");

    _id = id;
    _awaiting_bgp_id = false;
    if(!_awaiting_as && !_awaiting_bgp_id) {
	_bgp.local_config(_as, _id);
	_awaiting_config = false;	
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_bgp_id(
				 // Output values, 
				 IPv4& id) 
{
    if(_awaiting_bgp_id)
	return XrlCmdError::COMMAND_FAILED("BGP ID not yet configured");
    id = _id;
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_add_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port, 
	const uint32_t&	as, 
	const IPv4&	next_hop,
	const bool&     is_client,  
	const uint32_t&	holdtime)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u as %u"
	      " next_hop %s holdtime %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      XORP_UINT_CAST(as), next_hop.str().c_str(),
	      XORP_UINT_CAST(holdtime));

    if(_awaiting_config)
	return XrlCmdError::COMMAND_FAILED("BGP Not configured!!!");

    if(!_bgp.processes_ready())
	return XrlCmdError::COMMAND_FAILED("FEA or RIB not running");

    
    BGPPeerData *pd = 0;
    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	// Calculate PeerType
        AsNum asn(static_cast<uint16_t>(as)); 
	PeerType peer_type = PEER_TYPE_EBGP; 
        if (_bgp.get_local_data()->as() == asn) { 
             if (is_client) 
                  peer_type = PEER_TYPE_IBGP_CLIENT; 
             else 
                  peer_type = PEER_TYPE_IBGP; 
        } else if (_bgp.get_local_data()->in_confederation(asn)) { 
	       peer_type = PEER_TYPE_EBGP_CONFED;		  
        } 

	pd = new BGPPeerData(iptuple, asn, next_hop, holdtime, peer_type);

    } catch(XorpException& e) {
	delete pd;
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    if(!_bgp.create_peer(pd)) {
	delete pd;
	return XrlCmdError::COMMAND_FAILED();
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_delete_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port));

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.delete_peer(iptuple))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_enable_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port));

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.enable_peer(iptuple))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_disable_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port));

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.disable_peer(iptuple))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_change_local_ip(
	// Input values,
	const string&	local_ip,
	const uint32_t&	local_port,
	const string&	peer_ip,
	const uint32_t&	peer_port,
	const string&	new_local_ip)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u"
	      " new local ip %s\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      new_local_ip.c_str());

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.change_local_ip(iptuple, new_local_ip))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_change_local_port(
	// Input values,
	const string&	local_ip,
	const uint32_t&	local_port,
	const string&	peer_ip,
	const uint32_t&	peer_port,
	const uint32_t&	new_local_port)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u"
	      " new local port %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      new_local_port);

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.change_local_port(iptuple, new_local_port))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_change_peer_port(
	// Input values,
	const string&	local_ip,
	const uint32_t&	local_port,
	const string&	peer_ip,
	const uint32_t&	peer_port,
	const uint32_t&	new_peer_port)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u"
	      " new peer port %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      new_peer_port);

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.change_peer_port(iptuple, new_peer_port))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_set_peer_as(
				  // Input values,
				  const string&	local_ip,
				  const uint32_t& local_port,
				  const string&	peer_ip,
				  const uint32_t& peer_port,
				  const uint32_t& peer_as)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u"
	      " peer as %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      peer_as);

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.set_peer_as(iptuple, peer_as))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_set_holdtime(
				   // Input values,
				   const string&  local_ip,
				   const uint32_t& local_port,
				   const string& peer_ip,
				   const uint32_t& peer_port,
				   const uint32_t& holdtime)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u"
	      " new holdtime %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      holdtime);

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.set_holdtime(iptuple, holdtime))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_set_nexthop4(
				   // Input values,
				   const string& local_ip,
				   const uint32_t& local_port,
				   const string& peer_ip,
				   const uint32_t& peer_port,
				   const IPv4& next_hop)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u"
	      " nexthop %s\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      cstring(next_hop));

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.set_nexthop4(iptuple, next_hop))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_set_nexthop6(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port,
	const IPv6&	next_hop)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u "
	      " next-hop %s\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      cstring(next_hop));

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.set_nexthop6(iptuple, next_hop))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_nexthop6(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port,
	IPv6&	next_hop)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port));

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.get_nexthop6(iptuple, next_hop))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_set_peer_state(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port, 
	const bool& toggle)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u toggle %d\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port), toggle);

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.set_peer_state(iptuple, toggle))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_set_peer_md5_password(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port, 
	const string& password)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u password %s\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      password.c_str());

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.set_peer_md5_password(iptuple, password))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_activate(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port));

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.activate(iptuple))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_next_hop_rewrite_filter(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port,
	const IPv4& 	next_hop)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u "
	      "next hop %s\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      next_hop.str().c_str());

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.next_hop_rewrite_filter(iptuple, next_hop))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_originate_route4(
	// Input values,
	const IPv4Net&	nlri,
	const IPv4&	next_hop,
	const bool&	unicast,
	const bool&	multicast)
{
    debug_msg("nlri %s next hop %s unicast %d multicast %d\n",
	      nlri.str().c_str(), next_hop.str().c_str(), unicast, multicast);

    if (!_bgp.originate_route(nlri, next_hop, unicast, multicast,PolicyTags()))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_originate_route6(
	// Input values,
	const IPv6Net&	nlri,
	const IPv6&	next_hop,
	const bool&	unicast,
	const bool&	multicast)
{
    debug_msg("nlri %s next hop %s unicast %d multicast %d\n",
	      nlri.str().c_str(), next_hop.str().c_str(), unicast, multicast);

    if (!_bgp.originate_route(nlri, next_hop, unicast, multicast,PolicyTags()))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_withdraw_route4(
	// Input values,
	const IPv4Net&	nlri,
	const bool&	unicast,
	const bool&	multicast)
{
    debug_msg("nlri %s unicast %d multicast %d\n",
	      nlri.str().c_str(), unicast, multicast);

    if (!_bgp.withdraw_route(nlri, unicast, multicast))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_withdraw_route6(
	// Input values,
	const IPv6Net&	nlri,
	const bool&	unicast,
	const bool&	multicast)
{
    debug_msg("nlri %s unicast %d multicast %d\n",
	      nlri.str().c_str(), unicast, multicast);

    if (!_bgp.withdraw_route(nlri, unicast, multicast))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_list_start(
					  // Output values, 
					  uint32_t& token,
					  bool&	more)
{
    more = _bgp.get_peer_list_start(token);
    return  XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_list_next(
					 // Input values, 
					 const uint32_t&	token, 
					 // Output values, 
					 string&	local_ip, 
					 uint32_t&	local_port, 
					 string&	peer_ip, 
					 uint32_t&	peer_port, 
					 bool&	more)
{
    more = _bgp.get_peer_list_next(token, local_ip, local_port, peer_ip, 
				   peer_port);

    debug_msg("local ip %s local port %u peer ip %s peer port %u "
	      "token %#x %s\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      XORP_UINT_CAST(token),
	      more ? "more" : "");

    return  XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_id(
				  // Input values, 
				  const string&	local_ip, 
				  const uint32_t&	local_port, 
				  const string&	peer_ip, 
				  const uint32_t&	peer_port, 
				  // Output values, 
				  IPv4&	peer_id)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port));

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if (!_bgp.get_peer_id(iptuple, peer_id)) {
	    return XrlCmdError::COMMAND_FAILED();
	}
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_status(
				      // Input values, 
				      const string&	local_ip, 
				      const uint32_t&	local_port, 
				      const string&	peer_ip, 
				      const uint32_t&	peer_port, 
				      // Output values, 
				      uint32_t&	peer_state, 
				      uint32_t&	admin_status)
{

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if (!_bgp.get_peer_status(iptuple, peer_state, admin_status)) {
	    return XrlCmdError::COMMAND_FAILED();
	}
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_negotiated_version(
						  // Input values, 
						  const string& local_ip, 
						  const uint32_t& local_port, 
						  const string& peer_ip, 
						  const uint32_t& peer_port, 
						  // Output values, 
						  int32_t& neg_version)
{
    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if (!_bgp.get_peer_negotiated_version(iptuple, neg_version)) {
	    return XrlCmdError::COMMAND_FAILED();
	}
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_as(
				  // Input values, 
				  const string& local_ip, 
				  const uint32_t& local_port, 
				  const string&	peer_ip, 
				  const uint32_t& peer_port, 
				  // Output values, 
				  uint32_t& peer_as)
{
    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if (!_bgp.get_peer_as(iptuple, peer_as)) {
	    return XrlCmdError::COMMAND_FAILED();
	}
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_msg_stats(
					 // Input values, 
					 const string& local_ip, 
					 const uint32_t& local_port, 
					 const string& peer_ip, 
					 const uint32_t& peer_port, 
					 // Output values, 
					 uint32_t&	in_updates, 
					 uint32_t&	out_updates, 
					 uint32_t&	in_msgs, 
					 uint32_t&	out_msgs, 
					 uint32_t&	last_error, 
					 uint32_t&	in_update_elapsed)
{
    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	uint16_t last_error_short;
	if (!_bgp.get_peer_msg_stats(iptuple, in_updates, out_updates,
				     in_msgs, out_msgs, last_error_short, 
				     in_update_elapsed)) {
	    return XrlCmdError::COMMAND_FAILED();
	}
	last_error = last_error_short;
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_established_stats(
						 // Input values, 
						 const string& local_ip, 
						 const uint32_t& local_port, 
						 const string& peer_ip, 
						 const uint32_t& peer_port, 
						 // Output values, 
						 uint32_t& transitions, 
						 uint32_t& established_time)
{
    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if (!_bgp.get_peer_established_stats(iptuple, transitions,
					     established_time)) {
	    return XrlCmdError::COMMAND_FAILED();
	}
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_timer_config(
					    // Input values, 
					    const string&	local_ip, 
					    const uint32_t&	local_port, 
					    const string&	peer_ip, 
					    const uint32_t&	peer_port, 
					    // Output values, 
					    uint32_t& retry_interval, 
					    uint32_t& hold_time, 
					    uint32_t& keep_alive, 
					    uint32_t& hold_time_conf, 
					    uint32_t& keep_alive_conf, 
					    uint32_t& min_as_origin_interval,
					    uint32_t& min_route_adv_interval)
{
    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);
	if (!_bgp.get_peer_timer_config(iptuple, retry_interval, hold_time,
					keep_alive, hold_time_conf,
					keep_alive_conf, min_as_origin_interval,
					min_route_adv_interval)) {
	    return XrlCmdError::COMMAND_FAILED();
	}
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    return XrlCmdError::OKAY();
}


XrlCmdError
XrlBgpTarget::bgp_0_2_register_rib(
				   // Input values, 
				   const string&	name)
{
    debug_msg("%s\n", name.c_str());

    if(!_bgp.register_ribname(name)) {
	return XrlCmdError::COMMAND_FAILED(c_format(
			    "Couldn't register rib name %s",
					   name.c_str()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_get_v4_route_list_start(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	// Output values, 
	uint32_t& token)
{
    debug_msg("\n");

    if (_bgp.get_route_list_start<IPv4>(token, unicast, multicast)) {
	return XrlCmdError::OKAY();
    } else {
	return XrlCmdError::COMMAND_FAILED();
    }
}

XrlCmdError
XrlBgpTarget::bgp_0_2_get_v6_route_list_start(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	// Output values, 
	uint32_t& token)
{
    if (_bgp.get_route_list_start<IPv6>(token, unicast, multicast)) {
	return XrlCmdError::OKAY();
    } else {
	return XrlCmdError::COMMAND_FAILED();
    }
}

XrlCmdError
XrlBgpTarget::bgp_0_2_get_v4_route_list_next(
	// Input values, 
	const uint32_t&	token, 
	// Output values, 
	IPv4&	peer_id, 
	IPv4Net& net, 
	uint32_t& best_and_origin, 
	vector<uint8_t>& aspath, 
	IPv4& nexthop, 
	int32_t& med, 
	int32_t& localpref, 
	int32_t& atomic_agg, 
	vector<uint8_t>& aggregator, 
	int32_t& calc_localpref, 
	vector<uint8_t>& attr_unknown,
	bool& valid,
	bool& unicast,
	bool& multicast)
{
    debug_msg("\n");

    uint32_t origin;
    bool best = false;
    if (_bgp.get_route_list_next<IPv4>(token, peer_id, net, origin, aspath,
				       nexthop, med, localpref, atomic_agg,
				       aggregator, calc_localpref,
				       attr_unknown, best, unicast,
				       multicast)) {
	//trivial encoding to keep XRL arg count small enough
	if (best) {
	    best_and_origin = (2 << 16) | origin;
	} else {
	    best_and_origin = (1 << 16) | origin;
	}
	valid = true;
    } else {
	valid = false;
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_get_v6_route_list_next(
	// Input values, 
	const uint32_t&	token, 
	// Output values, 
	IPv4& peer_id, 
	IPv6Net& net, 
	uint32_t& best_and_origin, 
	vector<uint8_t>& aspath, 
	IPv6& nexthop, 
	int32_t& med, 
	int32_t& localpref, 
	int32_t& atomic_agg, 
	vector<uint8_t>& aggregator, 
	int32_t& calc_localpref, 
	vector<uint8_t>& attr_unknown,
	bool& valid,
	bool& unicast,
	bool& multicast)
{
    debug_msg("\n");

    uint32_t origin;
    bool best;
    if (_bgp.get_route_list_next<IPv6>(token, peer_id, net, origin, aspath,
				       nexthop, med, localpref, atomic_agg,
				       aggregator, calc_localpref,
				       attr_unknown, best, unicast,
				       multicast)) {
	//trivial encoding to keep XRL arg count small enough
	if (best) {
	    best_and_origin = (2 << 16) & origin;
	} else {
	    best_and_origin = (1 << 16) & origin;
	}
	valid = true;
    } else {
	valid = false;
    }
    return XrlCmdError::OKAY();
}

XrlCmdError XrlBgpTarget::rib_client_0_1_route_info_changed4(
        // Input values, 
        const IPv4& addr,
        const uint32_t& prefix_len,
	const IPv4&	nexthop, 
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin)
{
    IPNet<IPv4> net(addr, prefix_len);
    debug_msg("IGP route into changed for net %s\n", net.str().c_str());
    debug_msg("Nexthop: %s Metric: %u AdminDistance: %u ProtocolOrigin: %s\n",
	      nexthop.str().c_str(), XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(admin_distance), protocol_origin.c_str());
    
    // TODO: admin_distance and protocol_origin are not used
    if(!_bgp.rib_client_route_info_changed4(addr, prefix_len, nexthop, metric))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError XrlBgpTarget::rib_client_0_1_route_info_changed6(
	// Input values, 
	const IPv6&	addr, 
	const uint32_t&	prefix_len,
	const IPv6&	nexthop, 
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin)
{
    IPNet<IPv6> net(addr, prefix_len);
    debug_msg("IGP route into changed for net %s\n", net.str().c_str());
    debug_msg("Nexthop: %s Metric: %u AdminDistance: %u ProtocolOrigin: %s\n",
	      nexthop.str().c_str(), XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(admin_distance), protocol_origin.c_str());
    
    // TODO: admin_distance and protocol_origin are not used
    if(!_bgp.rib_client_route_info_changed6(addr, prefix_len, nexthop, metric))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError XrlBgpTarget::rib_client_0_1_route_info_invalid4(
	// Input values, 
	const IPv4&	addr, 
	const uint32_t&	prefix_len)
{
    IPNet<IPv4> net(addr, prefix_len);
    debug_msg("IGP route into changed for net %s\n", net.str().c_str());

    if(!_bgp.rib_client_route_info_invalid4(addr, prefix_len))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError XrlBgpTarget::rib_client_0_1_route_info_invalid6(
	// Input values, 
	const IPv6&	addr, 
	const uint32_t&	prefix_len)
{
    IPNet<IPv6> net(addr, prefix_len);
    debug_msg("IGP route into changed for net %s\n", net.str().c_str());

    if(!_bgp.rib_client_route_info_invalid6(addr, prefix_len))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError XrlBgpTarget::bgp_0_2_set_parameter(
				  // Input values,
				  const string&	local_ip, 
				  const uint32_t&	local_port, 
				  const string&	peer_ip, 
				  const uint32_t&	peer_port, 
				  const string& parameter,
				  const bool&	toggle)
{
    debug_msg("local ip %s local port %u peer ip %s peer port %u"
	      " parameter %s state %s\n",
	      local_ip.c_str(), XORP_UINT_CAST(local_port),
	      peer_ip.c_str(), XORP_UINT_CAST(peer_port),
	      parameter.c_str(), toggle ? "set" : "unset");

    try {
	Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(),
			peer_port);

	if(!_bgp.set_parameter(iptuple,parameter, toggle))
	    return XrlCmdError::COMMAND_FAILED();
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::finder_event_observer_0_1_xrl_target_birth(
	// Input values, 
	const string&	target_class, 
	const string&	target_instance)
{
    debug_msg("birth class %s instance %s\n", target_class.c_str(),
	      target_instance.c_str());

    _bgp.notify_birth(target_class, target_instance);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::finder_event_observer_0_1_xrl_target_death(
	// Input values, 
	const string&	target_class, 
	const string&	target_instance)
{
    debug_msg("death class %s instance %s\n", target_class.c_str(),
	      target_instance.c_str());

    _bgp.notify_death(target_class, target_instance);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::policy_backend_0_1_configure(const uint32_t& filter, 
					   const string& conf) {
    try {
	debug_msg("[BGP] policy filter: %d conf: %s\n", filter, conf.c_str());
	_bgp.configure_filter(filter,conf);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Filter configure failed: " +
					   e.str());
    }
    return XrlCmdError::OKAY();					   
}

XrlCmdError
XrlBgpTarget::policy_backend_0_1_reset(const uint32_t& filter) {
    try {
	_bgp.reset_filter(filter);
    } catch(const PolicyException& e){ 
	return XrlCmdError::COMMAND_FAILED("Filter reset failed: " +
					   e.str());
    }
    return XrlCmdError::OKAY();					   
}

XrlCmdError
XrlBgpTarget::policy_backend_0_1_push_routes() {
    debug_msg("[BGP] Route Push\n");
    _bgp.push_routes();
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::policy_redist4_0_1_add_route4(
        const IPv4Net&	    network,
        const bool&	    unicast,
        const bool&	    multicast,
        const IPv4&	    nexthop,
        const uint32_t&	    metric,
        const XrlAtomList&  policytags) {

    UNUSED(metric);
    _bgp.originate_route(network,nexthop,unicast,multicast,policytags);
    return XrlCmdError::OKAY();
	
}	
        
XrlCmdError 
XrlBgpTarget::policy_redist4_0_1_delete_route4(
        const IPv4Net&	network,
        const bool&     unicast,
        const bool&     multicast) {

    _bgp.withdraw_route(network,unicast,multicast);
    return XrlCmdError::OKAY();
}	
        
XrlCmdError 
XrlBgpTarget::policy_redist6_0_1_add_route6(
        const IPv6Net&	    network,
        const bool&	    unicast,
        const bool&	    multicast,
        const IPv6&	    nexthop,
        const uint32_t&	    metric,
        const XrlAtomList&  policytags) {
    
    UNUSED(metric);
    _bgp.originate_route(network,nexthop,unicast,multicast,policytags);
    return XrlCmdError::OKAY();
}	
        
XrlCmdError 
XrlBgpTarget::policy_redist6_0_1_delete_route6(
        const IPv6Net&  network,
        const bool&     unicast,
        const bool&     multicast) {
    
    _bgp.withdraw_route(network,unicast,multicast);
    return XrlCmdError::OKAY();
}	

XrlCmdError
XrlBgpTarget::profile_0_1_enable(const string& pname)
{
    debug_msg("profile variable %s\n", pname.c_str());
    try {
	_bgp.profile().enable(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::profile_0_1_disable(const string&	pname)
{
    debug_msg("profile variable %s\n", pname.c_str());
    try {
	_bgp.profile().disable(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::profile_0_1_get_entries(const string& pname,
				      const string& instance_name)
{
    debug_msg("profile variable %s instance %s\n", pname.c_str(),
	      instance_name.c_str());

    // Lock and initialize.
    try {
	_bgp.profile().lock_log(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    ProfileUtils::transmit_log(pname,
			       _bgp.get_router(), instance_name,
			       &_bgp.profile());

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::profile_0_1_clear(const string& pname)
{
    debug_msg("profile variable %s\n", pname.c_str());
    try {
	_bgp.profile().clear(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::profile_0_1_list(string& info)
{
    debug_msg("\n");
    
    info = _bgp.profile().list();
    return XrlCmdError::OKAY();
}

bool 
XrlBgpTarget::waiting()
{
    return _awaiting_config;
}

bool 
XrlBgpTarget::done()
{
    return _done;
}
