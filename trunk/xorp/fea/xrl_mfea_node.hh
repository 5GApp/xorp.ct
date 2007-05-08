// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_mfea_node.hh,v 1.31 2007/05/07 23:48:25 pavlin Exp $

#ifndef __FEA_XRL_MFEA_NODE_HH__
#define __FEA_XRL_MFEA_NODE_HH__


//
// MFEA (Multicast Forwarding Engine Abstraction) XRL-aware node definition.
//

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "xrl/interfaces/mfea_client_xif.hh"
#include "xrl/interfaces/cli_manager_xif.hh"
#include "xrl/targets/mfea_base.hh"

#include "libfeaclient_bridge.hh"
#include "mfea_node.hh"
#include "mfea_node_cli.hh"


//
// The top-level class that wraps-up everything together under one roof
//
class XrlMfeaNode : public MfeaNode,
		    public XrlStdRouter,
		    public XrlMfeaTargetBase,
		    public MfeaNodeCli {
public:
    XrlMfeaNode(FeaNode&	fea_node,
		int		family,
		xorp_module_id	module_id,
		EventLoop&	eventloop,
		const string&	class_name,
		const string&	finder_hostname,
		uint16_t	finder_port,
		const string&	finder_target);
    virtual ~XrlMfeaNode();

    /**
     * Startup the node operation.
     *
     * @return true on success, false on failure.
     */
    bool	startup();

    /**
     * Shutdown the node operation.
     *
     * @return true on success, false on failure.
     */
    bool	shutdown();

    /**
     * Get a reference to the XrlRouter instance.
     *
     * @return a reference to the XrlRouter (@ref XrlRouter) instance.
     */
    XrlRouter&	xrl_router() { return *this; }

    /**
     * Get a const reference to the XrlRouter instance.
     *
     * @return a const reference to the XrlRouter (@ref XrlRouter) instance.
     */
    const XrlRouter& xrl_router() const { return *this; }

    //
    // XrlMfeaNode front-end interface
    //
    int enable_cli();
    int disable_cli();
    int start_cli();
    int stop_cli();
    int enable_mfea();
    int disable_mfea();
    int start_mfea();
    int stop_mfea();
    
    
protected:
    //
    // XRL target methods
    //
    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(
	// Output values, 
	string&	name);

    /**
     *  Get version string from Xrl Target
     */
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
     *  Shutdown cleanly
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Announce target birth to observer.
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_birth(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    /**
     *  Announce target death to observer.
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_death(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    /**
     *  Process a CLI command.
     *  
     *  @param processor_name the processor name for this command.
     *  
     *  @param cli_term_name the terminal name the command was entered from.
     *  
     *  @param cli_session_id the CLI session ID the command was entered from.
     *  
     *  @param command_name the command name to process.
     *  
     *  @param command_args the command arguments to process.
     *  
     *  @param ret_processor_name the processor name to return back to the CLI.
     *  
     *  @param ret_cli_term_name the terminal name to return back.
     *  
     *  @param ret_cli_session_id the CLI session ID to return back.
     *  
     *  @param ret_command_output the command output to return back.
     */
    XrlCmdError cli_processor_0_1_process_command(
	// Input values, 
	const string&	processor_name, 
	const string&	cli_term_name, 
	const uint32_t& cli_session_id,
	const string&	command_name, 
	const string&	command_args, 
	// Output values, 
	string&	ret_processor_name, 
	string&	ret_cli_term_name, 
	uint32_t& ret_cli_session_id,
	string&	ret_command_output);

    /**
     *  Test if the underlying system supports IPv4 multicast routing.
     *  
     *  @param result true if the underlying system supports IPv4 multicast
     *  routing, otherwise false.
     */
    XrlCmdError mfea_0_1_have_multicast_routing4(
	// Output values, 
	bool&	result);

    /**
     *  Test if the underlying system supports IPv6 multicast routing.
     *  
     *  @param result true if the underlying system supports IPv6 multicast
     *  routing, otherwise false.
     */
    XrlCmdError mfea_0_1_have_multicast_routing6(
	// Output values, 
	bool&	result);

   /**
     *  Add/delete a protocol in the Multicast FEA.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol to add/delete.
     *  
     *  @param protocol_id the ID of the protocol to add/delete
     *  (both sides must agree on the particular values).
     */
    XrlCmdError mfea_0_1_add_protocol4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id);

    XrlCmdError mfea_0_1_add_protocol6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id);

    XrlCmdError mfea_0_1_delete_protocol4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id);

    XrlCmdError mfea_0_1_delete_protocol6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id);

    /**
     *  Start/stop a protocol on an interface in the Multicast FEA.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol to start/stop on the
     *  particular vif.
     *  
     *  @param protocol_id the ID of the protocol to add/stop on the particular
     *  vif (both sides must agree on the particular values).
     *  
     *  @param vif_name the name of the vif to start/stop for the particular
     *  protocol.
     *  
     *  @param vif_index the index of the vif to start/stop for the particular
     *  protocol.
     */
    XrlCmdError mfea_0_1_start_protocol_vif4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    XrlCmdError mfea_0_1_start_protocol_vif6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    XrlCmdError mfea_0_1_stop_protocol_vif4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    XrlCmdError mfea_0_1_stop_protocol_vif6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index);

    /**
     *  Enable/disable the receiving of kernel-originated signal messages.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol to add.
     *  
     *  @param protocol_id the ID of the protocol to add (both sides must agree
     *  on the particular values).
     *  
     *  @param is_allow if true, enable the receiving of kernel-originated
     *  signal messages by protocol
     */
    XrlCmdError mfea_0_1_allow_signal_messages(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const bool&	is_allow);
    
    /**
     *  Join/leave a multicast group.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol that joins/leave the
     *  group.
     *  
     *  @param protocol_id the ID of the protocol that joins/leave the group
     *  (both sides must agree on the particular values).
     *
     *  @param vif_name the name of the vif to join/leave the multicast group.
     *  
     *  @param vif_index the index of the vif to join/leave the multicast
     *  group.
     *  
     *  @param group_address the multicast group to join/leave
     */
    XrlCmdError mfea_0_1_join_multicast_group4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	group_address);

    XrlCmdError mfea_0_1_join_multicast_group6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	group_address);

    XrlCmdError mfea_0_1_leave_multicast_group4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	group_address);

    XrlCmdError mfea_0_1_leave_multicast_group6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	group_address);

    /**
     *  Add/delete a Multicast Forwarding Cache with the kernel.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param source_address the source address of the MFC to add/delete.
     *  
     *  @param group_address the group address of the MFC to add/delete.
     *  
     *  @param iif_vif_index the index of the vif that is the incoming
     *  interface.
     *  
     *  @param oiflist the bit-vector with the set of outgoing interfaces.
     *
     *  @param oiflist_disable_wrongvif the bit-vector with the set of outgoing
     *  interfaces to disable WRONGVIF kernel signal.
     *  
     *  @param max_vifs_oiflist the number of vifs covered by oiflist or
     *  oiflist_disable_wrongvif .
     *  
     *  @param rp_address the RP address of the MFC to add.
     */
    XrlCmdError mfea_0_1_add_mfc4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address, 
	const uint32_t&	iif_vif_index, 
	const vector<uint8_t>&	oiflist, 
	const vector<uint8_t>&	oiflist_disable_wrongvif, 
	const uint32_t&	max_vifs_oiflist, 
	const IPv4&	rp_address);

    XrlCmdError mfea_0_1_add_mfc6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address, 
	const uint32_t&	iif_vif_index, 
	const vector<uint8_t>&	oiflist, 
	const vector<uint8_t>&	oiflist_disable_wrongvif, 
	const uint32_t&	max_vifs_oiflist, 
	const IPv6&	rp_address);

    XrlCmdError mfea_0_1_delete_mfc4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address);

    XrlCmdError mfea_0_1_delete_mfc6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address);

    /**
     *  Send a protocol message to the MFEA.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param protocol_name the name of the protocol that sends a message.
     *  
     *  @param protocol_id the ID of the protocol that sends a message (both
     *  sides must agree on the particular values).
     *  
     *  @param vif_name the name of the vif to send the message.
     *  
     *  @param vif_index the vif index of the vif to send the message.
     *  
     *  @param source_address the address of the sender.
     *  
     *  @param dest_address the destination address.
     *  
     *  @param ip_ttl the TTL of the IP packet to send. If it has a negative
     *  value, the TTL will be set by the lower layers.
     *  
     *  @param ip_tos the TOS of the IP packet to send. If it has a negative
     *  value, the TOS will be set by the lower layers.
     *  
     *  @param is_router_alert set/reset the IP Router Alert option in the IP
     *  packet to send (when applicable).
     *
     *  @param ip_internet_control if true, then this is IP control traffic.
     */
    XrlCmdError mfea_0_1_send_protocol_message4(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv4&	source_address, 
	const IPv4&	dest_address, 
	const int32_t&	ip_ttl, 
	const int32_t&	ip_tos, 
	const bool&	is_router_alert, 
	const bool&	ip_internet_control,
	const vector<uint8_t>&	protocol_message);

    XrlCmdError mfea_0_1_send_protocol_message6(
	// Input values, 
	const string&	xrl_sender_name, 
	const string&	protocol_name, 
	const uint32_t&	protocol_id, 
	const string&	vif_name, 
	const uint32_t&	vif_index, 
	const IPv6&	source_address, 
	const IPv6&	dest_address, 
	const int32_t&	ip_ttl, 
	const int32_t&	ip_tos, 
	const bool&	is_router_alert, 
	const bool&	ip_internet_control,
	const vector<uint8_t>&	protocol_message);
    
    /**
     *  Add/delete a dataflow monitor with the MFEA.
     *  
     *  @param xrl_sender_name the XRL name of the originator of this XRL.
     *  
     *  @param source_address the source address of the dataflow to start/stop
     *  monitoring.
     *  
     *  @param group_address the group address of the dataflow to start/stop
     *  monitoring.
     *  
     *  @param threshold_interval_sec the number of seconds in the interval to
     *  measure.
     *  
     *  @param threshold_interval_usec the number of microseconds in the
     *  interval to measure.
     *  
     *  @param threshold_packets the threshold (in number of packets) to
     *  compare against.
     *  
     *  @param threshold_bytes the threshold (in number of bytes) to compare
     *  against.
     *  
     *  @param is_threshold_in_packets if true, threshold_packets is valid.
     *  
     *  @param is_threshold_in_bytes if true, threshold_bytes is valid.
     *  
     *  @param is_geq_upcall if true, the operation for comparison is ">=".
     *  
     *  @param is_leq_upcall if true, the operation for comparison is "<=".
     */
    XrlCmdError mfea_0_1_add_dataflow_monitor4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address, 
	const uint32_t&	threshold_interval_sec, 
	const uint32_t&	threshold_interval_usec, 
	const uint32_t&	threshold_packets, 
	const uint32_t&	threshold_bytes, 
	const bool&	is_threshold_in_packets, 
	const bool&	is_threshold_in_bytes, 
	const bool&	is_geq_upcall, 
	const bool&	is_leq_upcall);

    XrlCmdError mfea_0_1_add_dataflow_monitor6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address, 
	const uint32_t&	threshold_interval_sec, 
	const uint32_t&	threshold_interval_usec, 
	const uint32_t&	threshold_packets, 
	const uint32_t&	threshold_bytes, 
	const bool&	is_threshold_in_packets, 
	const bool&	is_threshold_in_bytes, 
	const bool&	is_geq_upcall, 
	const bool&	is_leq_upcall);

    XrlCmdError mfea_0_1_delete_dataflow_monitor4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address, 
	const uint32_t&	threshold_interval_sec, 
	const uint32_t&	threshold_interval_usec, 
	const uint32_t&	threshold_packets, 
	const uint32_t&	threshold_bytes, 
	const bool&	is_threshold_in_packets, 
	const bool&	is_threshold_in_bytes, 
	const bool&	is_geq_upcall, 
	const bool&	is_leq_upcall);

    XrlCmdError mfea_0_1_delete_dataflow_monitor6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address, 
	const uint32_t&	threshold_interval_sec, 
	const uint32_t&	threshold_interval_usec, 
	const uint32_t&	threshold_packets, 
	const uint32_t&	threshold_bytes, 
	const bool&	is_threshold_in_packets, 
	const bool&	is_threshold_in_bytes, 
	const bool&	is_geq_upcall, 
	const bool&	is_leq_upcall);

    XrlCmdError mfea_0_1_delete_all_dataflow_monitor4(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv4&	source_address, 
	const IPv4&	group_address);

    XrlCmdError mfea_0_1_delete_all_dataflow_monitor6(
	// Input values, 
	const string&	xrl_sender_name, 
	const IPv6&	source_address, 
	const IPv6&	group_address);

    /**
     *  Enable/disable/start/stop a MFEA vif interface.
     *
     *  @param vif_name the name of the vif to enable/disable/start/stop.
     *
     *  @param enable if true, then enable the vif, otherwise disable it.
     */
    XrlCmdError mfea_0_1_enable_vif(
	// Input values,
	const string&	vif_name,
	const bool&	enable);

    XrlCmdError mfea_0_1_start_vif(
	// Input values, 
	const string&	vif_name);

    XrlCmdError mfea_0_1_stop_vif(
	// Input values, 
	const string&	vif_name);

    /**
     *  Enable/disable/start/stop all MFEA vif interfaces.
     *
     *  @param enable if true, then enable the vifs, otherwise disable them.
     */
    XrlCmdError mfea_0_1_enable_all_vifs(
	// Input values,
	const bool&	enable);

    XrlCmdError mfea_0_1_start_all_vifs();

    XrlCmdError mfea_0_1_stop_all_vifs();

    /**
     *  Enable/disable/start/stop the MFEA.
     *
     *  @param enable if true, then enable the MFEA, otherwise disable it.
     */
    XrlCmdError mfea_0_1_enable_mfea(
	// Input values,
	const bool&	enable);

    XrlCmdError mfea_0_1_start_mfea();

    XrlCmdError mfea_0_1_stop_mfea();

    /**
     *  Enable/disable/start/stop the MFEA CLI access.
     *
     *  @param enable if true, then enable the MFEA CLI access, otherwise
     *  disable it.
     */
    XrlCmdError mfea_0_1_enable_cli(
	// Input values,
	const bool&	enable);

    XrlCmdError mfea_0_1_start_cli();

    XrlCmdError mfea_0_1_stop_cli();

    /**
     *  Enable/disable the MFEA trace log for all operations.
     *
     *  @param enable if true, then enable the trace log, otherwise disable it.
     */
    XrlCmdError mfea_0_1_log_trace_all(
	// Input values,
	const bool&	enable);

private:
    /**
     * Called when Finder connection is established.
     *
     * Note that this method overwrites an XrlRouter virtual method.
     */
    virtual void finder_connect_event();

    /**
     * Called when Finder disconnect occurs.
     *
     * Note that this method overwrites an XrlRouter virtual method.
     */
    virtual void finder_disconnect_event();

    //
    // Protocol node methods
    //
    int	proto_send(const string& dst_module_instance_name,
		   xorp_module_id dst_module_id,
		   uint32_t vif_index,
		   const IPvX& src, const IPvX& dst,
		   int ip_ttl, int ip_tos, bool is_router_alert,
		   bool ip_internet_control,
		   const uint8_t* sndbuf, size_t sndlen, string& error_msg);
    //
    // XXX: mfea_client_client_send_recv_protocol_message_cb() in fact
    // is the callback when proto_send() calls send_recv_protocol_message()
    // so the destination protocol will receive a protocol message.
    // Sigh, the 'recv' part in the name is rather confusing, but that
    // is in the name of consistency between the XRL calling function
    // and the return result callback...
    //
    void mfea_client_client_send_recv_protocol_message_cb(const XrlError& xrl_error);

    int signal_message_send(const string& dst_module_instance_name,
			    xorp_module_id dst_module_id,
			    int message_type,
			    uint32_t vif_index,
			    const IPvX& src, const IPvX& dst,
			    const uint8_t *rcvbuf, size_t rcvlen);
    //
    // XXX: mfea_client_client_send_recv_kernel_signal_message_cb() in fact
    // is the callback when signal_message_send() calls
    // send_recv_kernel_signal_message()
    // so the destination protocol will receive a kernel message.
    // Sigh, the 'recv' part in the name is rather confusing, but that
    // is in the name of consistency between the XRL calling function
    // and the return result callback...
    //
    void mfea_client_client_send_recv_kernel_signal_message_cb(const XrlError& xrl_error);

    int dataflow_signal_send(const string& dst_module_instance_name,
			     xorp_module_id dst_module_id,
			     const IPvX& source_addr,
			     const IPvX& group_addr,
			     uint32_t threshold_interval_sec,
			     uint32_t threshold_interval_usec,
			     uint32_t measured_interval_sec,
			     uint32_t measured_interval_usec,
			     uint32_t threshold_packets,
			     uint32_t threshold_bytes,
			     uint32_t measured_packets,
			     uint32_t measured_bytes,
			     bool is_threshold_in_packets,
			     bool is_threshold_in_bytes,
			     bool is_geq_upcall,
			     bool is_leq_upcall);
    
    void mfea_client_client_send_recv_dataflow_signal_cb(const XrlError& xrl_error);

    /**
     * Send a message to a client to add a configured vif.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @param vif_name the name of the vif to add.
     * @param vif_index the vif index of the vif to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	send_add_config_vif(const string& dst_module_instance_name,
			    xorp_module_id dst_module_id,
			    const string& vif_name,
			    uint32_t vif_index);
    
    /**
     * Send a message to a client to delete a configured vif.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @param vif_name the name of the vif to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	send_delete_config_vif(const string& dst_module_instance_name,
			       xorp_module_id dst_module_id,
			       const string& vif_name);
    
    /**
     * Send a message to a client to add an address to a configured vif.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @param vif_name the name of the vif.
     * @param addr the address to add.
     * @param subnet the subnet address to add.
     * @param broadcast the broadcast address to add.
     * @param peer the peer address to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	send_add_config_vif_addr(const string& dst_module_instance_name,
				 xorp_module_id dst_module_id,
				 const string& vif_name,
				 const IPvX& addr,
				 const IPvXNet& subnet,
				 const IPvX& broadcast,
				 const IPvX& peer);
    
    /**
     * Send a message to a client to delete an address from a configured vif.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @param vif_name the name of the vif.
     * @param addr the address to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	send_delete_config_vif_addr(const string& dst_module_instance_name,
				    xorp_module_id dst_module_id,
				    const string& vif_name,
				    const IPvX& addr);
    
    /**
     * Send a message to a client to set the vif flags of a configured vif.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @param vif_name the name of the vif.
     * @param is_pim_register true if the vif is a PIM Register interface.
     * @param is_p2p true if the vif is point-to-point interface.
     * @param is_loopback true if the vif is a loopback interface.
     * @param is_multicast true if the vif is multicast capable.
     * @param is_broadcast true if the vif is broadcast capable.
     * @param is_up true if the underlying vif is UP.
     * @param mtu the MTU of the vif.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	send_set_config_vif_flags(const string& dst_module_instance_name,
				  xorp_module_id dst_module_id,
				  const string& vif_name,
				  bool is_pim_register,
				  bool is_p2p,
				  bool is_loopback,
				  bool is_multicast,
				  bool is_broadcast,
				  bool is_up,
				  uint32_t mtu);
    
    /**
     * Send a message to a client to complete the set of vif configuration
     * changes.
     * 
     * @param dst_module_instance_name the name of the protocol
     * instance-destination of the message.
     * @param dst_module_id the module ID of the protocol-destination
     * of the message.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int	send_set_config_all_vifs_done(const string& dst_module_instance_name,
				      xorp_module_id dst_module_id);
    
    void mfea_client_client_send_new_vif_cb(const XrlError& xrl_error);
    void mfea_client_client_send_delete_vif_cb(const XrlError& xrl_error);
    void mfea_client_client_send_add_vif_addr_cb(const XrlError& xrl_error);
    void mfea_client_client_send_delete_vif_addr_cb(const XrlError& xrl_error);
    void mfea_client_client_send_set_vif_flags_cb(const XrlError& xrl_error);
    void mfea_client_client_send_set_all_vifs_done_cb(const XrlError& xrl_error);
    
    //
    // Protocol node CLI methods
    //
    int add_cli_command_to_cli_manager(const char *command_name,
				       const char *command_help,
				       bool is_command_cd,
				       const char *command_cd_prompt,
				       bool is_command_processor);
    void cli_manager_client_send_add_cli_command_cb(const XrlError& xrl_error);
    int delete_cli_command_from_cli_manager(const char *command_name);
    void cli_manager_client_send_delete_cli_command_cb(const XrlError& xrl_error);
    
    const string& my_xrl_target_name() {
	return XrlMfeaTargetBase::name();
    }
    
    int family() const { return (MfeaNode::family()); }

    EventLoop&			_eventloop;
    const string		_class_name;
    const string		_instance_name;
    const string		_finder_target;

    XrlMfeaClientV0p1Client	_xrl_mfea_client_client;
    XrlCliManagerV0p1Client	_xrl_cli_manager_client;
    XrlFinderEventNotifierV0p1Client	_xrl_finder_client;

    LibFeaClientBridge		_lib_mfea_client_bridge;    // The MFEA clients

    bool			_is_finder_alive;
};

#endif // __FEA_XRL_MFEA_NODE_HH__
