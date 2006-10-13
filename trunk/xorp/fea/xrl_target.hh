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

// $XORP: xorp/fea/xrl_target.hh,v 1.59 2006/06/15 06:04:36 pavlin Exp $

#ifndef __FEA_XRL_TARGET_HH__
#define __FEA_XRL_TARGET_HH__

#include "xrl/targets/fea_base.hh"
#include "xrl_fti.hh"
#include "xrl_ifmanager.hh"

class FtiConfig;
class InterfaceManager;
class LibFeaClientBridge;
class XrlIfConfigUpdateReporter;
class XrlRawSocket4Manager;
class XrlRawSocket6Manager;
class XrlSocketServer;
class Profile;

class XrlFeaTarget : public XrlFeaTargetBase {
public:
    XrlFeaTarget(EventLoop&			e,
		 XrlRouter&			rtr,
		 FtiConfig& 			ftic,
		 InterfaceManager& 		ifmgr,
		 XrlIfConfigUpdateReporter&	ifupd,
		 Profile&			profile,
		 XrlRawSocket4Manager*		xrsm4	= 0,
		 XrlRawSocket6Manager*		xrsm6	= 0,
		 LibFeaClientBridge*		lfbr	= 0,
		 XrlSocketServer*		xss	= 0);

    bool done() const { return _done; }

    XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    /**
     *  Get status from Xrl Target
     */
    XrlCmdError common_0_1_get_status(
	// Output values,
        uint32_t& status,
	string&	reason);

    /**
     * Shutdown FEA cleanly
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Enable/disable Click FEA support.
     *
     *  @param enable if true, then enable the Click FEA support, otherwise
     *  disable it.
     */
    XrlCmdError fea_click_0_1_enable_click(
	// Input values,
	const bool&	enable);

    /**
     *  Start Click FEA support.
     */
    XrlCmdError fea_click_0_1_start_click();

    /**
     *  Stop Click FEA support.
     */
    XrlCmdError fea_click_0_1_stop_click();

    /**
     *  Enable/disable duplicating the Click routes to the system kernel.
     *
     *  @param enable if true, then enable duplicating the Click routes to the
     *  system kernel, otherwise disable it.
     */
    XrlCmdError fea_click_0_1_enable_duplicate_routes_to_kernel(
	// Input values,
	const bool&	enable);

    /**
     *  Enable/disable kernel-level Click FEA support.
     *
     *  @param enable if true, then enable the kernel-level Click FEA support,
     *  otherwise disable it.
     */
    XrlCmdError fea_click_0_1_enable_kernel_click(
	// Input values,
	const bool&	enable);

    /**
     *  Enable/disable installing kernel-level Click on startup.
     *
     *  @param enable if true, then install kernel-level Click on startup.
     */
    XrlCmdError fea_click_0_1_enable_kernel_click_install_on_startup(
	// Input values,
	const bool&	enable);

    /**
     *  Specify the list of kernel Click modules to load on startup if
     *  installing kernel-level Click on startup is enabled. The file names of
     *  the kernel modules are separated by colon.
     *
     *  @param modules the list of kernel Click modules (separated by colon) to
     *  load.
     */
    XrlCmdError fea_click_0_1_set_kernel_click_modules(
	// Input values,
	const string&	modules);

    /**
     *  Specify the kernel-level Click mount directory.
     *
     *  @param directory the kernel-level Click mount directory.
     */
    XrlCmdError fea_click_0_1_set_kernel_click_mount_directory(
	// Input values,
	const string&	directory);

    /**
     *  Specify the external program to generate the kernel-level Click
     *  configuration.
     *
     *  @param kernel_click_config_generator_file the name of the external
     *  program to generate the kernel-level Click configuration.
     */
    XrlCmdError fea_click_0_1_set_kernel_click_config_generator_file(
	// Input values,
	const string&	kernel_click_config_generator_file);

    /**
     *  Enable/disable user-level Click FEA support.
     *
     *  @param enable if true, then enable the user-level Click FEA support,
     *  otherwise disable it.
     */
    XrlCmdError fea_click_0_1_enable_user_click(
	// Input values,
	const bool&	enable);

    /**
     *  Specify the user-level Click command file.
     *
     *  @param user_click_command_file the name of the user-level Click command
     *  file.
     */
    XrlCmdError fea_click_0_1_set_user_click_command_file(
	// Input values,
	const string&	user_click_command_file);

    /**
     *  Specify the extra arguments to the user-level Click command.
     *
     *  @param user_click_command_extra_arguments the extra arguments to the
     *  user-level Click command.
     */
    XrlCmdError fea_click_0_1_set_user_click_command_extra_arguments(
	// Input values,
	const string&	user_click_command_extra_arguments);

    /**
     *  Specify whether to execute on startup the user-level Click command.
     *
     *  @param user_click_command_execute_on_startup if true, then execute the
     *  user-level Click command on startup.
     */
    XrlCmdError fea_click_0_1_set_user_click_command_execute_on_startup(
	// Input values,
	const bool&	user_click_command_execute_on_startup);

    /**
     *  Specify the socket port to use for control access to the user-level
     *  Click.
     *
     *  @param user_click_control_socket_port the socket port to use for
     *  control access to the user-level Click.
     */
    XrlCmdError fea_click_0_1_set_user_click_control_socket_port(
	// Input values,
	const uint32_t&	user_click_control_socket_port);

    /**
     *  Specify the address to use for control access to the user-level
     *  Click.
     *
     *  @param user_click_control_address the address to use for
     *  control access to the user-level Click.
     */
    XrlCmdError fea_click_0_1_set_user_click_control_address(
	// Input values,
	const IPv4&	user_click_control_address);

    /**
     *  Specify the configuration file to be used by user-level Click on
     *  startup.
     *
     *  @param user_click_startup_config_file the name of the configuration
     *  file to be used by user-level Click on startup.
     */
    XrlCmdError fea_click_0_1_set_user_click_startup_config_file(
	// Input values,
	const string&	user_click_startup_config_file);

    /**
     *  Specify the external program to generate the user-level Click
     *  configuration.
     *
     *  @param user_click_config_generator_file the name of the external
     *  program to generate the user-level Click configuration.
     */
    XrlCmdError fea_click_0_1_set_user_click_config_generator_file(
	// Input values,
	const string&	user_click_config_generator_file);

    /**
     *  Add a FIB client.
     *
     *  @param client_target_name the target name of the FIB client to add.
     *  @param send_updates whether updates should be sent.
     *  @param send_resolves whether resolve requests should be sent.
     */
    XrlCmdError fea_fib_0_1_add_fib_client4(
	// Input values,
	const string&	client_target_name,
	const bool&	send_updates,
	const bool&	send_resolves);

    XrlCmdError fea_fib_0_1_add_fib_client6(
	// Input values,
	const string&	client_target_name,
	const bool&	send_updates,
	const bool&	send_resolves);

    /**
     *  Delete a FIB client.
     *
     *  @param target_name the target name of the FIB client to delete.
     */
    XrlCmdError fea_fib_0_1_delete_fib_client4(
	// Input values,
	const string&	client_target_name);

    XrlCmdError fea_fib_0_1_delete_fib_client6(
	// Input values,
	const string&	client_target_name);

    //
    // FEA network interface management interface
    //

    XrlCmdError ifmgr_0_1_set_restore_original_config_on_shutdown(
	// Input values,
	const bool&	enable);

    XrlCmdError ifmgr_0_1_get_system_interface_names(
	// Output values,
	XrlAtomList&	ifnames);

    XrlCmdError ifmgr_0_1_get_configured_interface_names(
	// Output values,
	XrlAtomList&	ifnames);

    XrlCmdError ifmgr_0_1_get_system_vif_names(
	const string&		ifname,
	// Output values,
	XrlAtomList&		ifnames);

    XrlCmdError ifmgr_0_1_get_configured_vif_names(
	const string&	ifname,
	// Output values,
	XrlAtomList&		ifnames);

    XrlCmdError ifmgr_0_1_get_system_vif_flags(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	bool&	enabled,
	bool&	broadcast,
	bool&	loopback,
	bool&	point_to_point,
	bool&	multicast);

    XrlCmdError ifmgr_0_1_get_configured_vif_flags(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	bool&	enabled,
	bool&	broadcast,
	bool&	loopback,
	bool&	point_to_point,
	bool&	multicast);

    XrlCmdError ifmgr_0_1_get_system_vif_pif_index(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	uint32_t&	pif_index);

    XrlCmdError ifmgr_0_1_get_configured_vif_pif_index(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	uint32_t&	pif_index);

    XrlCmdError ifmgr_0_1_start_transaction(
	// Output values,
	uint32_t&	tid);

    XrlCmdError ifmgr_0_1_commit_transaction(
	// Input values,
	const uint32_t&	tid);

    XrlCmdError ifmgr_0_1_abort_transaction(
	// Input values,
	const uint32_t&	tid);

    XrlCmdError ifmgr_0_1_create_interface(
	// Input values,
	const uint32_t& tid,
	const string&	ifname);

    XrlCmdError ifmgr_0_1_delete_interface(
	// Input values,
	const uint32_t& tid,
	const string&	ifname);

    /**
     *  Implicitly configure an interface within the FEA by using information
     *  from the underlying system.
     *
     *  @param tid the transaction ID.
     *
     *  @param ifname the name of the interface to configure.
     */
    XrlCmdError ifmgr_0_1_configure_interface_from_system(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname);

    XrlCmdError ifmgr_0_1_set_interface_enabled(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const bool&	enabled);

    XrlCmdError ifmgr_0_1_get_system_interface_enabled(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_get_configured_interface_enabled(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_set_interface_discard(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const bool&	discard);

    XrlCmdError ifmgr_0_1_get_configured_interface_discard(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		discard);

    XrlCmdError ifmgr_0_1_get_system_interface_discard(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		discard);

    XrlCmdError ifmgr_0_1_set_mac(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const Mac&	mac);

    XrlCmdError ifmgr_0_1_restore_original_mac(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname);

    XrlCmdError ifmgr_0_1_get_system_mac(
	// Input values,
	const string&	ifname,
	// Output values,
	Mac&	mac);

    XrlCmdError ifmgr_0_1_get_configured_mac(
	// Input values,
	const string&	ifname,
	// Output values,
	Mac&	mac);

    XrlCmdError ifmgr_0_1_set_mtu(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const uint32_t&	mtu);

    XrlCmdError ifmgr_0_1_restore_original_mtu(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname);

    XrlCmdError ifmgr_0_1_get_system_mtu(
	// Input values,
	const string&	ifname,
	// Output values,
	uint32_t&	mtu);

    XrlCmdError ifmgr_0_1_get_configured_mtu(
	// Input values,
	const string&	ifname,
	// Output values,
	uint32_t&	mtu);

    XrlCmdError ifmgr_0_1_get_system_no_carrier(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		no_carrier);

    XrlCmdError ifmgr_0_1_get_configured_no_carrier(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		no_carrier);

    XrlCmdError ifmgr_0_1_get_system_address_flags4(
	// Input values,
	const string& ifname,
	const string& vifname,
	const IPv4&   address,
	// Output values,
	bool& up,
	bool& broadcast,
	bool& loopback,
	bool& point_to_point,
	bool& multicast);

    XrlCmdError ifmgr_0_1_get_system_address_flags6(
	// Input values,
	const string& ifname,
	const string& vifname,
	const IPv6&   address,
	// Output values,
	bool& up,
	bool& loopback,
	bool& point_to_point,
	bool& multicast);

    XrlCmdError ifmgr_0_1_get_configured_address_flags4(
	// Input values,
	const string& ifname,
	const string& vifname,
	const IPv4&   address,
	// Output values,
	bool& up,
	bool& broadcast,
	bool& loopback,
	bool& point_to_point,
	bool& multicast);

    XrlCmdError ifmgr_0_1_get_configured_address_flags6(
	// Input values,
	const string& ifname,
	const string& vifname,
	const IPv6&   address,
	// Output values,
	bool& up,
	bool& loopback,
	bool& point_to_point,
	bool& multicast);

    XrlCmdError ifmgr_0_1_create_vif(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif);

    XrlCmdError ifmgr_0_1_delete_vif(
	// Input values,
	const uint32_t& tid,
	const string& 	ifname,
	const string&	vif);

    XrlCmdError ifmgr_0_1_set_vif_enabled(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const bool&	enabled);

    XrlCmdError ifmgr_0_1_get_system_vif_enabled(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_get_configured_vif_enabled(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_get_system_vif_addresses4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	XrlAtomList&	addresses);

    XrlCmdError ifmgr_0_1_get_configured_vif_addresses4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	XrlAtomList&	addresses);

    XrlCmdError ifmgr_0_1_create_address4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address);

    XrlCmdError ifmgr_0_1_delete_address4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address);

    XrlCmdError ifmgr_0_1_set_address4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address);

    XrlCmdError ifmgr_0_1_set_address_enabled4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	const bool&	en);

    XrlCmdError ifmgr_0_1_get_system_address_enabled4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_get_configured_address_enabled4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_set_prefix4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	const uint32_t&	prefix_len);

    XrlCmdError ifmgr_0_1_get_system_prefix4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	uint32_t&	prefix_len);

    XrlCmdError ifmgr_0_1_get_configured_prefix4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	uint32_t&	prefix_len);

    XrlCmdError ifmgr_0_1_set_broadcast4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	const IPv4&	broadcast);

    XrlCmdError ifmgr_0_1_get_system_broadcast4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	IPv4&		broadcast);

    XrlCmdError ifmgr_0_1_get_configured_broadcast4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	IPv4&		broadcast);

    XrlCmdError ifmgr_0_1_set_endpoint4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	const IPv4&	endpoint);

    XrlCmdError ifmgr_0_1_get_system_endpoint4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	IPv4&	endpoint);

    XrlCmdError ifmgr_0_1_get_configured_endpoint4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	IPv4&	endpoint);

    XrlCmdError ifmgr_0_1_get_system_vif_addresses6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	XrlAtomList&	addresses);

    XrlCmdError ifmgr_0_1_get_configured_vif_addresses6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	XrlAtomList&	addresses);

    XrlCmdError ifmgr_0_1_create_address6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address);

    XrlCmdError ifmgr_0_1_delete_address6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address);

    XrlCmdError ifmgr_0_1_set_address_enabled6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	const bool&	enabled);

    XrlCmdError ifmgr_0_1_get_system_address_enabled6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_get_configured_address_enabled6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_set_prefix6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	const uint32_t&	prefix_len);

    XrlCmdError ifmgr_0_1_get_system_prefix6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	// Output values,
	uint32_t&	prefix_len);

    XrlCmdError ifmgr_0_1_get_configured_prefix6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	// Output values,
	uint32_t&	prefix_len);

    XrlCmdError ifmgr_0_1_set_endpoint6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	const IPv6&	endpoint);

    XrlCmdError ifmgr_0_1_get_system_endpoint6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	// Output values,
	IPv6&		endpoint);

    XrlCmdError ifmgr_0_1_get_configured_endpoint6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	// Output values,
	IPv6&		endpoint);

    XrlCmdError ifmgr_0_1_register_client(
	// Input values,
	const string&	spyname);

    XrlCmdError ifmgr_0_1_unregister_client(
	// Input values,
	const string&	spyname);

    XrlCmdError ifmgr_0_1_register_system_interfaces_client(
	// Input values,
	const string&	spyname);

    XrlCmdError ifmgr_0_1_unregister_system_interfaces_client(
	// Input values,
	const string&	spyname);

    XrlCmdError ifmgr_replicator_0_1_register_ifmgr_mirror(
	// Input values,
	const string&	clientname);

    XrlCmdError ifmgr_replicator_0_1_unregister_ifmgr_mirror(
	// Input values,
	const string&	clientname);

    //
    // Forwarding Table Interface
    //

    XrlCmdError fti_0_2_lookup_route_by_dest4(
	// Input values,
	const IPv4&	host,
	// Output values,
	IPv4Net&	netmask,
	IPv4&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin);

    XrlCmdError fti_0_2_lookup_route_by_dest6(
	// Input values,
	const IPv6&	host,
	// Output values,
	IPv6Net&	netmask,
	IPv6&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin);

    XrlCmdError fti_0_2_lookup_route_by_network4(
	// Input values,
	const IPv4Net&	dst,
	// Output values,
	IPv4&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin);

    XrlCmdError fti_0_2_lookup_route_by_network6(
	// Input values,
	const IPv6Net&	dst,
	// Output values,
	IPv6&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin);

    XrlCmdError fti_0_2_have_ipv4(
	// Output values, 
	bool&	result);

    XrlCmdError fti_0_2_have_ipv6(
	// Output values, 
	bool&	result);

    XrlCmdError fti_0_2_get_unicast_forwarding_enabled4(
	// Output values,
	bool&	enabled);

    XrlCmdError fti_0_2_get_unicast_forwarding_enabled6(
	// Output values,
	bool&	enabled);

    XrlCmdError fti_0_2_set_unicast_forwarding_enabled4(
	// Input values,
	const bool&	enabled);

    XrlCmdError fti_0_2_set_unicast_forwarding_enabled6(
	// Input values,
	const bool&	enabled);

    //
    // RIB routes redistribution transaction-based XRL interface
    //

    /**
     *  Start transaction.
     *
     *  @param tid the transaction ID to use for this transaction.
     */
    XrlCmdError redist_transaction4_0_1_start_transaction(
	// Output values,
	uint32_t&	tid);

    /**
     *  Commit transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction4_0_1_commit_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Abort transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction4_0_1_abort_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Add/delete a routing entry.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param dst destination network.
     *
     *  @param nexthop nexthop router address.
     *
     *  @param ifname interface name associated with nexthop.
     *
     *  @param vifname virtual interface name with nexthop.
     *
     *  @param metric origin routing protocol metric for route.
     *
     *  @param admin_distance administrative distance of origin routing
     *  protocol.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     *
     *  @param protocol_origin the name of the protocol that originated this
     *  routing entry.
     */
    XrlCmdError redist_transaction4_0_1_add_route(
	// Input values,
	const uint32_t&	tid,
	const IPv4Net&	dst,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    XrlCmdError redist_transaction4_0_1_delete_route(
	// Input values,
	const uint32_t&	tid,
	const IPv4Net&	dst,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    /**
     *  Delete all routing entries.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     */
    XrlCmdError redist_transaction4_0_1_delete_all_routes(
	// Input values,
	const uint32_t&	tid,
	const string&	cookie);

    /**
     *  Start transaction.
     *
     *  @param tid the transaction ID to use for this transaction.
     */
    XrlCmdError redist_transaction6_0_1_start_transaction(
	// Output values,
	uint32_t&	tid);

    /**
     *  Commit transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction6_0_1_commit_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Abort transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction6_0_1_abort_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Add/delete a routing entry.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param dst destination network.
     *
     *  @param nexthop nexthop router address.
     *
     *  @param ifname interface name associated with nexthop.
     *
     *  @param vifname virtual interface name with nexthop.
     *
     *  @param metric origin routing protocol metric for route.
     *
     *  @param admin_distance administrative distance of origin routing
     *  protocol.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     *
     *  @param protocol_origin the name of the protocol that originated this
     *  routing entry.
     */
    XrlCmdError redist_transaction6_0_1_add_route(
	// Input values,
	const uint32_t&	tid,
	const IPv6Net&	dst,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    XrlCmdError redist_transaction6_0_1_delete_route(
	// Input values,
	const uint32_t&	tid,
	const IPv6Net&	dst,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    /**
     *  Delete all routing entries.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     */
    XrlCmdError redist_transaction6_0_1_delete_all_routes(
	// Input values,
	const uint32_t&	tid,
	const string&	cookie);

    //
    // IPv4 Raw Socket Server Interface
    //

    /**
     *  Send an IPv4 packet on a raw socket.
     *
     *  @param if_name the interface to send the packet on. It is essential for
     *  multicast. In the unicast case this field may be empty.
     *
     *  @param vif_name the vif to send the packet on. It is essential for
     *  multicast. In the unicast case this field may be empty.
     *
     *  @param src_address the IP source address.
     *
     *  @param dst_address the IP destination address.
     *
     *  @param ip_protocol the IP protocol number. It must be between 1 and
     *  255.
     *
     *  @param ip_ttl the IP TTL (hop-limit). If it has a negative value, the
     *  TTL will be set internally before transmission.
     *
     *  @param ip_tos the Type Of Service (Diffserv/ECN bits for IPv4). If it
     *  has a negative value, the TOS will be set internally before
     *  transmission.
     *
     *  @param ip_router_alert if true, then add the IP Router Alert option to
     *  the IP packet.
     *
     *  @param payload the payload, everything after the IP header and options.
     */
    XrlCmdError raw_packet4_0_1_send(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const IPv4&	src_address,
	const IPv4&	dst_address,
	const uint32_t&	ip_protocol,
	const int32_t&	ip_ttl,
	const int32_t&	ip_tos,
	const bool&	ip_router_alert,
	const vector<uint8_t>&	payload);

    /**
     *  Register to receive IPv4 packets. The receiver is expected to support
     *  raw_packet4_client/0.1 interface.
     *
     *  @param xrl_target_name the receiver's XRL target name.
     *
     *  @param if_name the interface through which packets should be accepted.
     *
     *  @param vif_name the vif through which packets should be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is
     *  interested in. It must be between 0 and 255. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param enable_multicast_loopback if true then enable delivering of
     *  multicast datagrams back to this host (assuming the host is a member of
     *  the same multicast group.
     */
    XrlCmdError raw_packet4_0_1_register_receiver(
	// Input values,
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const bool&	enable_multicast_loopback);

    /**
     *  Unregister to receive IPv4 packets.
     *
     *  @param xrl_target_name the receiver's XRL target name.
     *
     *  @param if_name the interface through which packets should not be
     *  accepted.
     *
     *  @param vif_name the vif through which packets should not be accepted.
     *
     *  @param ip_protocol the IP Protocol number that the receiver is not
     *  interested in anymore. It must be between 0 and 255. A protocol number
     *  of 0 is used to specify all protocols.
     */
    XrlCmdError raw_packet4_0_1_unregister_receiver(
	// Input values,
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol);

    /**
     *  Join an IPv4 multicast group.
     *
     *  @param xrl_target_name the receiver's XRL target name.
     *
     *  @param if_name the interface through which packets should be accepted.
     *
     *  @param vif_name the vif through which packets should be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is
     *  interested in. It must be between 0 and 255. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param group_address the multicast group address to join.
     */
    XrlCmdError raw_packet4_0_1_join_multicast_group(
	// Input values,
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const IPv4&	group_address);

    /**
     *  Leave an IPv4 multicast group.
     *
     *  @param xrl_target_name the receiver's XRL target name.
     *
     *  @param if_name the interface through which packets should not be
     *  accepted.
     *
     *  @param vif_name the vif through which packets should not be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is not
     *  interested in anymore. It must be between 0 and 255. A protocol number
     *  of 0 is used to specify all protocols.
     *
     *  @param group_address the multicast group address to leave.
     */
    XrlCmdError raw_packet4_0_1_leave_multicast_group(
	// Input values,
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const IPv4&	group_address);

    //
    // IPv6 Raw Socket Server Interface
    //

    /**
     *  Send an IPv6 packet on a raw socket.
     *
     *  @param if_name the interface to send the packet on. It is essential for
     *  multicast. In the unicast case this field may be empty.
     *
     *  @param vif_name the vif to send the packet on. It is essential for
     *  multicast. In the unicast case this field may be empty.
     *
     *  @param src_address the IP source address.
     *
     *  @param dst_address the IP destination address.
     *
     *  @param ip_protocol the IP protocol number. It must be between 1 and
     *  255.
     *
     *  @param ip_ttl the IP TTL (hop-limit). If it has a negative value, the
     *  TTL will be set internally before transmission.
     *
     *  @param ip_tos the Type Of Service (IP traffic class for IPv6). If it
     *  has a negative value, the TOS will be set internally before
     *  transmission.
     *
     *  @param ip_router_alert if true, then add the IP Router Alert option to
     *  the IP packet.
     *
     *  @param ext_headers_type a list of u32 integers with the types of the
     *  optional extention headers.
     *
     *  @param ext_headers_payload a list of payload data, one for each
     *  optional extention header. The number of entries must match
     *  ext_headers_type.
     *
     *  @param payload the payload, everything after the IP header and options.
     */
    XrlCmdError raw_packet6_0_1_send(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const IPv6&	src_address,
	const IPv6&	dst_address,
	const uint32_t&	ip_protocol,
	const int32_t&	ip_ttl,
	const int32_t&	ip_tos,
	const bool&	ip_router_alert,
	const XrlAtomList&	ext_headers_type,
	const XrlAtomList&	ext_headers_payload,
	const vector<uint8_t>&	payload);

    /**
     *  Register to receive IPv6 packets. The receiver is expected to support
     *  raw_packet6_client/0.1 interface.
     *
     *  @param xrl_target_name the receiver's XRL target name.
     *
     *  @param if_name the interface through which packets should be accepted.
     *
     *  @param vif_name the vif through which packets should be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is
     *  interested in. It must be between 0 and 255. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param enable_multicast_loopback if true then enable delivering of
     *  multicast datagrams back to this host (assuming the host is a member of
     *  the same multicast group.
     */
    XrlCmdError raw_packet6_0_1_register_receiver(
	// Input values,
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const bool&	enable_multicast_loopback);

    /**
     *  Unregister to receive IPv6 packets.
     *
     *  @param xrl_target_name the receiver's XRL target name.
     *
     *  @param if_name the interface through which packets should not be
     *  accepted.
     *
     *  @param vif_name the vif through which packets should not be accepted.
     *
     *  @param ip_protocol the IP Protocol number that the receiver is not
     *  interested in anymore. It must be between 0 and 255. A protocol number
     *  of 0 is used to specify all protocols.
     */
    XrlCmdError raw_packet6_0_1_unregister_receiver(
	// Input values,
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol);

    /**
     *  Join an IPv6 multicast group.
     *
     *  @param xrl_target_name the receiver's XRL target name.
     *
     *  @param if_name the interface through which packets should be accepted.
     *
     *  @param vif_name the vif through which packets should be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is
     *  interested in. It must be between 0 and 255. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param group_address the multicast group address to join.
     */
    XrlCmdError raw_packet6_0_1_join_multicast_group(
	// Input values,
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const IPv6&	group_address);

    /**
     *  Leave an IPv6 multicast group.
     *
     *  @param xrl_target_name the receiver's XRL target name.
     *
     *  @param if_name the interface through which packets should not be
     *  accepted.
     *
     *  @param vif_name the vif through which packets should not be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is not
     *  interested in anymore. It must be between 0 and 255. A protocol number
     *  of 0 is used to specify all protocols.
     *
     *  @param group_address the multicast group address to leave.
     */
    XrlCmdError raw_packet6_0_1_leave_multicast_group(
	// Input values,
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const IPv6&	group_address);

    //
    // Socket Locator interface(s)
    //
    XrlCmdError socket4_locator_0_1_find_socket_server_for_addr(
	// Input value
	const IPv4& addr,
	// Output value
	string&	xrl_target);

    XrlCmdError socket6_locator_0_1_find_socket_server_for_addr(
	// Input value
	const IPv6& addr,
	// Output value
	string&	xrl_target);

    XrlCmdError profile_0_1_enable(
	// Input values,
	const string&	pname);

    XrlCmdError profile_0_1_disable(
	// Input values,
	const string&	pname);

    XrlCmdError profile_0_1_get_entries(
	// Input values,
	const string&	pname,
	const string&	instance_name);

    XrlCmdError profile_0_1_clear(
	// Input values,
	const string&	pname);

    XrlCmdError profile_0_1_list(
	// Output values,
	string&	info);
private:
    bool have_ipv4() const { return (_have_ipv4); };
    bool have_ipv6() const { return (_have_ipv6); };

    XrlRouter&		       	_xrl_router;
    XrlFtiTransactionManager	_xftm;
    XrlInterfaceManager 	_xifmgr;
    XrlIfConfigUpdateReporter&	_xifcur;
    Profile&			_profile;
    XrlRawSocket4Manager*	_xrsm4;
    XrlRawSocket6Manager*	_xrsm6;
    LibFeaClientBridge*		_lfcb;
    XrlSocketServer*		_xss;

    bool			_done;		// True if we are done
    bool			_have_ipv4;	// True if we have IPv4
    bool			_have_ipv6;	// True if we have IPv6
};

#endif //  __FEA_XRL_TARGET_HH__
