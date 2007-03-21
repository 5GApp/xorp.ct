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

// $XORP: xorp/fea/xrl_packet_acl.hh,v 1.4 2006/11/07 18:55:40 pavlin Exp $

#ifndef __FEA_XRL_PACKET_ACL_HH__
#define __FEA_XRL_PACKET_ACL_HH__

#include <map>

#include "libxorp/xlog.h"
#include "libxipc/xrl_cmd_map.hh"

#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"
#include "libxipc/xrl_router.hh"

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_transaction.hh"
#include "xrl/targets/packet_acl_base.hh"


/**
 * Helper class for helping with packet ACL configuration transactions
 * via an Xrl interface.
 *
 * The class provides error messages suitable for Xrl return values
 * and does some extra checking not in the PaTransactionManager class.
 */
class XrlPacketAclTarget : public XrlPacketAclTargetBase {
protected:
    /**
     * Used to hold state for clients reading snapshots of the ACL tables.
     */
    struct PaBrowseState {
    public:
	PaBrowseState(const XrlPacketAclTarget& xpa,
		      const XorpTimer& timeout_timer,
		      const PaSnapshot4* snap4,
		      size_t idx = 0);
	PaBrowseState(const XrlPacketAclTarget& xpa,
		      const PaSnapshot4* snap4,
		      size_t idx = 0);
	~PaBrowseState();
	inline size_t index() const		{ return _idx; }
	inline void set_index(size_t idx)	{ _idx = idx; }
	inline const PaSnapshot4* snap4() const	{ return _snap4; }
	void defer_timeout();
	void cancel_timeout();
    private:
	const XrlPacketAclTarget&	_xpa;
	XorpTimer			_timeout_timer;
	const PaSnapshot4*		_snap4;	// XXX: not refcnted!
	size_t				_idx;
    };

    typedef map<uint32_t, PaBrowseState> PaBrowseDB;

    EventLoop&			_e;
    PaTransactionManager&	_pat;
    uint32_t			_browse_timeout_ms;

    uint32_t			_next_token;
    PaBrowseDB			_bdb;

    void crank_token();
    void timeout_browse(uint32_t token);

public:
    inline const EventLoop& eventloop() const { return _e; }
    inline uint32_t browse_timeout_ms() const { return _browse_timeout_ms; }

    enum { BROWSE_TIMEOUT_MS = 15000 };

    /**
     * Constructor.
     *
     * @param eventloop an EventLoop which will be used for scheduling timers.
     * @param cmds an XrlCmdMap that the commands associated with the target
     *		   should be added to.  This is typically the XrlRouter
     *		   associated with the target.
     * @param pat a PaTransactionManager which manages accesses to the
     *		  underlying ACL tables.
     */
    XrlPacketAclTarget(XrlCmdMap* cmds, EventLoop& e,
		       PaTransactionManager& pat,
		       uint32_t browse_timeout_ms = BROWSE_TIMEOUT_MS);

    /**
     * Destructor.
     *
     * Dissociates instance commands from command map.
     */
     virtual ~XrlPacketAclTarget();

    /**
     * Set command map.
     *
     * @param cmds pointer to command map to associate commands with.  This
     * argument is typically a pointer to the XrlRouter associated with the
     * target.
     *
     * @return true on success, false if cmds is null or a command map has
     * already been supplied.
     */
    bool set_command_map(XrlCmdMap* cmds);

    /**
     * Get Xrl instance name associated with command map.
     */
    inline const string& name() const { return _cmds->name(); }

    /**
     * Get version string of instance.
     */
    inline const char* version() const { return "packet_acl/0.1"; }

protected:
    /* ---------------------------------------------------------------- */
    /* XRL common interface methods */

    /**
     *  Function that needs to be implemented to:
     *
     *  Get name of Xrl Target
     */
     XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get version string from Xrl Target
     */
     XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    /**
     *  Function that needs to be implemented to:
     *
     *  Get status of Xrl Target
     */
     XrlCmdError common_0_1_get_status(
	// Output values,
	uint32_t&	status,
	string&	reason);

    /**
     *  Function that needs to be implemented to:
     *
     *  Request clean shutdown of Xrl Target
     */
     XrlCmdError common_0_1_shutdown();

    /* ---------------------------------------------------------------- */
    /* XRL packet_acl interface: global methods */

    /**
     *  Function that needs to be implemented to:
     *  Get the name of the ACL back-end provider currently in use.
     */
     XrlCmdError packet_acl_0_1_get_backend(
	// Output values,
	string&	name);

    /**
     *  Function that needs to be implemented to:
     *  Set the underlying packet ACL provider type in use. NOTE: If XORP rules
     *  currently exist, this operation will perform an implicit flush and
     *  reload when switching to the new provider.
     */
     XrlCmdError packet_acl_0_1_set_backend(
	// Input values,
	const string&	name);

    /**
     *  Function that needs to be implemented to:
     *  Get the underlying packet ACL provider version in use.
     */
     XrlCmdError packet_acl_0_1_get_version(
	// Output values,
	string&	version);

    /* ---------------------------------------------------------------- */
    /* XRL packet_acl interface: transaction control methods */

    /**
     *  Function that needs to be implemented to:
     *  Start an ACL configuration transaction.
     *
     *  @param tid The number of the newly started transaction.
     */
    XrlCmdError packet_acl_0_1_start_transaction(
	// Output values,
	uint32_t& tid);

    /**
     *  Function that needs to be implemented to:
     *  Commit a previously started ACL configuration transaction.
     *
     *  @param tid The number of the transaction to commit.
     */
    XrlCmdError packet_acl_0_1_commit_transaction(
	// Input values,
	const uint32_t& tid);

    /**
     *  Function that needs to be implemented to:
     *  Abort an ACL configuration transaction in progress.
     *
     *  @param tid The number of the transaction to abort.
     */
    XrlCmdError packet_acl_0_1_abort_transaction(
	// Input values,
	const uint32_t& tid);

    /* ---------------------------------------------------------------- */
    /* XRL packet_acl interface: per-transaction methods */

    /**
     *  Function that needs to be implemented to:
     *  Add an IPv6 family ACL entry.
     *
     *  @param tid The number of the transaction for this operation.
     *  @param ifname Name of the interface where this filter is to be applied.
     *  @param vifname Name of the vif where this filter is to be applied.
     *  @param src Source IPv6 address with network prefix.
     *  @param dst Destination IPv6 address with network prefix.
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     *  @param action Action to take when this ACL entry is matched.
     */
    XrlCmdError packet_acl_0_1_add_entry4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src,
	const IPv4Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport,
	const string&	action);

    /**
     *  Function that needs to be implemented to:
     *  Delete an IPv4 family ACL entry.
     *
     *  @param tid The number of the transaction for this operation.
     *  @param ifname Name of the interface where this filter is to be deleted.
     *  @param vifname Name of the vif where this filter is to be deleted.
     *  @param src Source IPv4 address with network prefix.
     *  @param dst Destination IPv4 address with network prefix.
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     */
     XrlCmdError packet_acl_0_1_delete_entry4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src,
	const IPv4Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport);

    /**
     *  Function that needs to be implemented to:
     *  Delete all IPv4 family ACL entries.
     *
     *  @param tid The number of the transaction for this operation.
     */
     XrlCmdError packet_acl_0_1_delete_all_entries4(
	// Input values,
	const uint32_t&	tid);

    XrlCmdError packet_acl_0_1_get_entry_list_start4(
	// Output values,
	uint32_t&	token,
	bool&		more);

    XrlCmdError packet_acl_0_1_get_entry_list_next4(
	// Input values,
	const uint32_t&	token,
	// Output values,
	string&		ifname,
	string&		vifname,
	IPv4Net&	src,
	IPv4Net&	dst,
	uint32_t&	proto,
	uint32_t&	sport,
	uint32_t&	dport,
	string&		action,
	bool&		more);

    /**
     *  Function that needs to be implemented to:
     *  Add an IPv6 family ACL entry.
     *
     *  @param tid The number of the transaction for this operation.
     *  @param ifname Name of the interface where this filter is to be applied.
     *  @param vifname Name of the vif where this filter is to be applied.
     *  @param src Source IPv6 address with network prefix.
     *  @param dst Destination IPv6 address with network prefix.
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     *  @param action Action to take when this filter is matched.
     */
     XrlCmdError packet_acl_0_1_add_entry6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src,
	const IPv6Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport,
	const string&	action);

    /**
     *  Function that needs to be implemented to:
     *  Delete an IPv6 family ACL entry.
     *
     *  @param tid The number of the transaction for this operation.
     *  @param ifname Name of the interface where this filter is to be deleted.
     *  @param vifname Name of the vif where this filter is to be deleted.
     *  @param src Source IPv6 address with network prefix.
     *  @param dst Destination IPv6 address with network prefix.
     *  @param proto IP protocol number for match (0-255, 255 is wildcard).
     *  @param sport Source TCP/UDP port (0-65535, 0 is wildcard).
     *  @param dport Destination TCP/UDP port (0-65535, 0 is wildcard).
     */
     XrlCmdError packet_acl_0_1_delete_entry6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src,
	const IPv6Net&	dst,
	const uint32_t&	proto,
	const uint32_t&	sport,
	const uint32_t&	dport);

    /**
     *  Function that needs to be implemented to:
     *  Delete all IPv6 family ACL entries.
     *
     *  @param tid The number of the transaction for this operation.
     */
     XrlCmdError packet_acl_0_1_delete_all_entries6(
	// Input values,
	const uint32_t&	tid);
};

#endif /* __FEA_XRL_PACKET_ACL_HH__ */
