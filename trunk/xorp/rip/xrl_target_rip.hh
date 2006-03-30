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

// $XORP: xorp/rip/xrl_target_rip.hh,v 1.20 2006/03/16 00:05:55 pavlin Exp $

#ifndef __RIP_XRL_TARGET_RIP_HH__
#define __RIP_XRL_TARGET_RIP_HH__

#include "libxorp/status_codes.h"
#include "xrl/targets/rip_base.hh"

class XrlRouter;
class XrlProcessSpy;

template<typename A> class System;
template<typename A> class XrlPortManager;
template<typename A> class XrlRipCommonTarget;
template<typename A> class XrlRedistManager;

class XrlRipTarget : public XrlRipTargetBase {
public:
    XrlRipTarget(EventLoop&			e,
		 XrlRouter& 			xr,
		 XrlProcessSpy& 		xps,
		 XrlPortManager<IPv4>&		xpm,
		 XrlRedistManager<IPv4>& 	xrm,
		 bool& 				should_exit,
		 System<IPv4>&			rip_system);
    ~XrlRipTarget();

    void set_status(ProcessStatus ps, const string& annotation = "");

    XrlCmdError common_0_1_get_target_name(string& name);
    XrlCmdError common_0_1_get_version(string& version);
    XrlCmdError common_0_1_get_status(uint32_t& status, string& reason);
    XrlCmdError common_0_1_shutdown();

    XrlCmdError
    finder_event_observer_0_1_xrl_target_birth(const string& class_name,
					       const string& instance_name);

    XrlCmdError
    finder_event_observer_0_1_xrl_target_death(const string& class_name,
					       const string& instance_name);

    XrlCmdError
    rip_0_1_add_rip_address(const string&	ifname,
			    const string&	vifname,
			    const IPv4&		addr);

    XrlCmdError
    rip_0_1_remove_rip_address(const string&	ifname,
			       const string&	vifname,
			       const IPv4&	addr);

    XrlCmdError
    rip_0_1_set_rip_address_enabled(const string&	ifname,
				    const string&	vifname,
				    const IPv4&		addr,
				    const bool&		enabled);

    XrlCmdError
    rip_0_1_rip_address_enabled(const string&	ifname,
				const string&	vifname,
				const IPv4&	addr,
				bool&		enabled);

    XrlCmdError rip_0_1_set_cost(const string&		ifname,
				 const string&		vifname,
				 const IPv4&		addr,
				 const uint32_t&	cost);

    XrlCmdError rip_0_1_cost(const string&	ifname,
			     const string&	vifname,
			     const IPv4&	addr,
			     uint32_t&		cost);

    XrlCmdError rip_0_1_set_horizon(const string&	ifname,
				    const string&	vifname,
				    const IPv4&		addr,
				    const string&	horizon);

    XrlCmdError rip_0_1_horizon(const string&	ifname,
				const string&	vifname,
				const IPv4&	addr,
				string&		horizon);

    XrlCmdError rip_0_1_set_passive(const string&	ifname,
				    const string&	vifname,
				    const IPv4&		addr,
				    const bool&		passive);

    XrlCmdError rip_0_1_passive(const string&	ifname,
				const string&	vifname,
				const IPv4&	addr,
				bool&		passive);

    XrlCmdError
    rip_0_1_set_accept_non_rip_requests(const string&	ifname,
					const string&	vifname,
					const IPv4&	addr,
					const bool&	accept);

    XrlCmdError rip_0_1_accept_non_rip_requests(const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						bool&		accept);

    XrlCmdError rip_0_1_set_accept_default_route(const string&	ifname,
						 const string&	vifname,
						 const IPv4&	addr,
						 const bool&	accept);

    XrlCmdError rip_0_1_accept_default_route(const string&	ifname,
					     const string&	vifname,
					     const IPv4&	addr,
					     bool&		accept);

    XrlCmdError
    rip_0_1_set_advertise_default_route(const string&	ifname,
					const string&	vifname,
					const IPv4&	addr,
					const bool&	advertise);

    XrlCmdError rip_0_1_advertise_default_route(const string&	ifname,
						const string&	vifname,
						const IPv4&	addr,
						bool&		advertise);

    XrlCmdError
    rip_0_1_set_route_expiry_seconds(const string&	ifname,
				     const string&	vifname,
				     const IPv4&	addr,
				     const uint32_t&	t_secs);

    XrlCmdError
    rip_0_1_route_expiry_seconds(const string&	ifname,
				 const string&	vifname,
				 const IPv4&	addr,
				 uint32_t&	t_secs);

    XrlCmdError
    rip_0_1_set_route_deletion_seconds(const string&	ifname,
				       const string&	vifname,
				       const IPv4&	addr,
				       const uint32_t&	t_secs);

    XrlCmdError
    rip_0_1_route_deletion_seconds(const string&	ifname,
				   const string&	vifname,
				   const IPv4&		addr,
				   uint32_t&		t_secs);

    XrlCmdError
    rip_0_1_set_table_request_seconds(const string&	ifname,
				      const string&	vifname,
				      const IPv4&	addr,
				      const uint32_t&	t_secs);

    XrlCmdError
    rip_0_1_table_request_seconds(const string&		ifname,
				  const string&		vifname,
				  const IPv4&		addr,
				  uint32_t&		t_secs);

    XrlCmdError
    rip_0_1_set_unsolicited_response_min_seconds(const string&	ifname,
						 const string&	vifname,
						 const IPv4&	addr,
						 const uint32_t& t_secs);

    XrlCmdError
    rip_0_1_unsolicited_response_min_seconds(const string&	ifname,
					     const string&	vifname,
					     const IPv4&	addr,
					     uint32_t&		t_secs);

    XrlCmdError
    rip_0_1_set_unsolicited_response_max_seconds(const string&	ifname,
						 const string&	vifname,
						 const IPv4&	addr,
						 const uint32_t& t_secs);

    XrlCmdError
    rip_0_1_unsolicited_response_max_seconds(const string&	ifname,
					     const string&	vifname,
					     const IPv4&	addr,
					     uint32_t&		t_secs);

    XrlCmdError
    rip_0_1_set_triggered_update_min_seconds(const string&	ifname,
					     const string&	vifname,
					     const IPv4&	addr,
					     const uint32_t&	t_secs);

    XrlCmdError
    rip_0_1_triggered_update_min_seconds(const string&	ifname,
					 const string&	vifname,
					 const IPv4&	addr,
					 uint32_t&	t_secs);

    XrlCmdError
    rip_0_1_set_triggered_update_max_seconds(const string&	ifname,
					     const string&	vifname,
					     const IPv4&	addr,
					     const uint32_t&	t_secs);

    XrlCmdError
    rip_0_1_triggered_update_max_seconds(const string&	ifname,
					 const string&	vifname,
					 const IPv4&	addr,
					 uint32_t&	t_secs);

    XrlCmdError
    rip_0_1_set_interpacket_delay_milliseconds(const string&	ifname,
					       const string&	vifname,
					       const IPv4&	addr,
					       const uint32_t&	t_msecs);

    XrlCmdError
    rip_0_1_interpacket_delay_milliseconds(const string&	ifname,
					   const string&	vifname,
					   const IPv4&		addr,
					   uint32_t&		t_msecs);

    XrlCmdError rip_0_1_set_simple_authentication_key(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const string&	password);

    XrlCmdError rip_0_1_delete_simple_authentication_key(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr);

    XrlCmdError rip_0_1_set_md5_authentication_key(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const uint32_t&	key_id,
	const string&	password,
	const string&	start_time,
	const string&	end_time);

    XrlCmdError rip_0_1_delete_md5_authentication_key(
	// Input values,
	const string&	ifname,
	const string&	vifname,
	const IPv4&	addr,
	const uint32_t&	key_id);

    XrlCmdError rip_0_1_rip_address_status(const string&	ifname,
					   const string&	vifname,
					   const IPv4&		addr,
					   string&		status);

    XrlCmdError rip_0_1_get_all_addresses(XrlAtomList&	ifnames,
					  XrlAtomList&	vifnames,
					  XrlAtomList&	addrs);

    XrlCmdError rip_0_1_get_peers(const string&		ifname,
				  const string&		vifname,
				  const IPv4&		addr,
				  XrlAtomList&		peers);

    XrlCmdError rip_0_1_get_all_peers(XrlAtomList&	peers,
				      XrlAtomList&	ifnames,
				      XrlAtomList&	vifnames,
				      XrlAtomList&	addrs);

    XrlCmdError rip_0_1_get_counters(const string&	ifname,
				     const string&	vifname,
				     const IPv4&	addr,
				     XrlAtomList&	descriptions,
				     XrlAtomList&	values);

    XrlCmdError rip_0_1_get_peer_counters(const string&	ifname,
					  const string&	vifname,
					  const IPv4&	addr,
					  const IPv4&	peer,
					  XrlAtomList&	descriptions,
					  XrlAtomList&	values,
					  uint32_t&	peer_last_active);

    XrlCmdError rip_0_1_redist_protocol_routes(const string&	protocol_name,
					       const uint32_t&	cost,
					       const uint32_t&	tag);

    XrlCmdError rip_0_1_no_redist_protocol_routes(const string&	protocol_name);

    XrlCmdError redist4_0_1_add_route(const IPv4Net&		net,
				      const IPv4&		nexthop,
				      const string&		ifname,
				      const string&		vifname,
				      const uint32_t&		metric,
				      const uint32_t&		ad,
				      const string&		cookie,
				      const string&		protocol_origin);

    XrlCmdError redist4_0_1_delete_route(const IPv4Net&		net,
					 const IPv4&		nexthop,
					 const string&		ifname,
					 const string&		vifname,
					 const uint32_t&	metric,
					 const uint32_t&	ad,
					 const string&		cookie,
					 const string&		protocol_origin);

    XrlCmdError redist4_0_1_starting_route_dump(const string&	cookie);

    XrlCmdError redist4_0_1_finishing_route_dump(const string&	cookie );

    XrlCmdError socket4_user_0_1_recv_event(const string&	sockid,
					    const IPv4&		src_host,
					    const uint32_t&	src_port,
					    const vector<uint8_t>& pdata);

    XrlCmdError socket4_user_0_1_connect_event(const string&	sockid,
					       const IPv4&	src_host,
					       const uint32_t&	src_port,
					       const string&	new_sockid,
					       bool&		accept);

    XrlCmdError socket4_user_0_1_error_event(const string&	sockid,
					     const string& 	reason,
					     const bool&	fatal);

    XrlCmdError socket4_user_0_1_close_event(const string&	sockid,
					     const string&	reason);


    XrlCmdError policy_backend_0_1_configure(
        // Input values,
        const uint32_t& filter,
        const string&   conf);

    XrlCmdError policy_backend_0_1_reset(
        // Input values,
        const uint32_t& filter);

    XrlCmdError policy_backend_0_1_push_routes();

    XrlCmdError policy_redist4_0_1_add_route4(
        // Input values,
        const IPv4Net&  network,
        const bool&     unicast,
        const bool&     multicast,
        const IPv4&     nexthop,
        const uint32_t& metric,
        const XrlAtomList&      policytags);

    XrlCmdError policy_redist4_0_1_delete_route4(
        // Input values,
        const IPv4Net&  network,
        const bool&     unicast,
        const bool&     multicast);

protected:
    EventLoop& 			_e;
    XrlRipCommonTarget<IPv4>* 	_ct;
};

#endif // __RIP_XRL_TARGET_RIP_HH__
