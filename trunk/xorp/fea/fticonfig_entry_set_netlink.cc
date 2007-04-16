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

#ident "$XORP: xorp/fea/fticonfig_entry_set_netlink.cc,v 1.34 2007/02/16 22:45:39 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "fticonfig.hh"
#include "fticonfig_entry_set.hh"
#include "netlink_socket_utils.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is netlink(7) sockets.
//


FtiConfigEntrySetNetlink::FtiConfigEntrySetNetlink(FtiConfig& ftic)
    : FtiConfigEntrySet(ftic),
      NetlinkSocket(ftic.eventloop()),
      _ns_reader(*(NetlinkSocket *)this)
{
#ifdef HAVE_NETLINK_SOCKETS
    register_ftic_primary();
#endif
}

FtiConfigEntrySetNetlink::~FtiConfigEntrySetNetlink()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to set "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FtiConfigEntrySetNetlink::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (NetlinkSocket::start(error_msg) < 0)
	return (XORP_ERROR);
    
    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigEntrySetNetlink::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (NetlinkSocket::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

bool
FtiConfigEntrySetNetlink::add_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetNetlink::delete_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

bool
FtiConfigEntrySetNetlink::add_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetNetlink::delete_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

#ifndef HAVE_NETLINK_SOCKETS
bool
FtiConfigEntrySetNetlink::add_entry(const FteX& )
{
    return false;
}

bool
FtiConfigEntrySetNetlink::delete_entry(const FteX& )
{
    return false;
}

#else // HAVE_NETLINK_SOCKETS
bool
FtiConfigEntrySetNetlink::add_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rtmsg)
	+ 3*sizeof(struct rtattr) + sizeof(int) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct rtmsg*	rtmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket&	ns = *this;
    int			family = fte.net().af();
    uint32_t		if_index = 0;
    void*		rta_align_data;

    debug_msg("add_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! ftic().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);

    if (fte.is_connected_route())
	return true;	// XXX: don't add/remove directly-connected routes

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtmsg));
    nlh->nlmsg_type = RTM_NEWROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    rtmsg = static_cast<struct rtmsg*>(NLMSG_DATA(nlh));
    rtmsg->rtm_family = family;
    rtmsg->rtm_dst_len = fte.net().prefix_len(); // The destination mask length
    rtmsg->rtm_src_len = 0;
    rtmsg->rtm_tos = 0;
    rtmsg->rtm_table = RT_TABLE_MAIN;
    rtmsg->rtm_protocol = RTPROT_XORP;		// Mark this as a XORP route
    rtmsg->rtm_scope = RT_SCOPE_UNIVERSE;
    rtmsg->rtm_type = RTN_UNICAST;
    rtmsg->rtm_flags = RTM_F_NOTIFY;

    // Add the destination address as an attribute
    rta_len = RTA_LENGTH(fte.net().masked_addr().addr_bytelen());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = RTM_RTA(rtmsg);
    rtattr->rta_type = RTA_DST;
    rtattr->rta_len = rta_len;
    data = static_cast<uint8_t*>(RTA_DATA(rtattr));
    fte.net().masked_addr().copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    // Add the nexthop address as an attribute
    if (fte.nexthop() != IPvX::ZERO(family)) {
	rta_len = RTA_LENGTH(fte.nexthop().addr_bytelen());
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
	}
	rta_align_data = reinterpret_cast<char*>(rtattr)
	    + RTA_ALIGN(rtattr->rta_len);
	rtattr = static_cast<struct rtattr*>(rta_align_data);
	rtattr->rta_type = RTA_GATEWAY;
	rtattr->rta_len = rta_len;
	data = static_cast<uint8_t*>(RTA_DATA(rtattr));
	fte.nexthop().copy_out(data);
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    }

    // Get the interface index, if it exists.
    if_index = 0;
#ifdef HAVE_IF_NAMETOINDEX
    if_index = if_nametoindex(fte.vifname().c_str());
#endif // HAVE_IF_NAMETOINDEX
    if (if_index == 0) {
	do {
	    //
	    // Check for a discard route; the referenced ifname must have
	    // the discard property. These use a separate route type in netlink
	    // land. Unlike BSD, it need not reference a loopback interface.
	    // Because this interface exists only in the FEA, it has no index.
	    //
	    if (fte.ifname().empty())
		break;
	    const IfTree& iftree = ftic().iftree();
	    IfTree::IfMap::const_iterator ii = iftree.get_if(fte.ifname());
	    XLOG_ASSERT(ii != iftree.ifs().end());

	    if (ii->second.discard()) {
		rtmsg->rtm_type = RTN_BLACKHOLE;
	    } else {
		// Catchall.
		XLOG_FATAL("Could not find interface index for name %s",
			   fte.vifname().c_str());
	    }
	    break;
	} while (false);
    }

    // If the interface has an index in the host stack, add it
    // as an attribute.
    if (if_index != 0) {
	int int_if_index = if_index;
	rta_len = RTA_LENGTH(sizeof(int_if_index));
	if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	    XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		       XORP_UINT_CAST(sizeof(buffer)),
		       XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
	}
	rta_align_data = reinterpret_cast<char*>(rtattr)
	    + RTA_ALIGN(rtattr->rta_len);
	rtattr = static_cast<struct rtattr*>(rta_align_data);
	rtattr->rta_type = RTA_OIF;
	rtattr->rta_len = rta_len;
	data = static_cast<uint8_t*>(RTA_DATA(rtattr));
	memcpy(data, &int_if_index, sizeof(int_if_index));
	nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;
    }

    // Add the route priority as an attribute
    int int_priority = fte.metric();
    rta_len = RTA_LENGTH(sizeof(int_priority));
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rta_align_data = reinterpret_cast<char*>(rtattr)
	+ RTA_ALIGN(rtattr->rta_len);
    rtattr = static_cast<struct rtattr*>(rta_align_data);
    rtattr->rta_type = RTA_PRIORITY;
    rtattr->rta_len = rta_len;
    data = static_cast<uint8_t*>(RTA_DATA(rtattr));
    memcpy(data, &int_priority, sizeof(int_priority));
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    //
    // XXX: the Linux kernel doesn't keep the admin distance, hence
    // we don't add it.
    //
    
    string error_msg;
    int last_errno = 0;
    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("Error writing to netlink socket: %s", strerror(errno));
	return false;
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg) < 0) {
	XLOG_ERROR("Error checking netlink request: %s", error_msg.c_str());
	return false;
    }

    return true;
}

bool
FtiConfigEntrySetNetlink::delete_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rtmsg)
	+ 3*sizeof(struct rtattr) + sizeof(int) + 512;
    union {
	uint8_t		data[buffer_size];
	struct nlmsghdr	nlh;
    } buffer;
    struct nlmsghdr*	nlh = &buffer.nlh;
    struct sockaddr_nl	snl;
    struct rtmsg*	rtmsg;
    struct rtattr*	rtattr;
    int			rta_len;
    uint8_t*		data;
    NetlinkSocket&	ns = *this;
    int			family = fte.net().af();

    debug_msg("delete_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! ftic().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);

    if (fte.is_connected_route())
	return true;	// XXX: don't add/remove directly-connected routes

    memset(&buffer, 0, sizeof(buffer));

    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = 0;

    //
    // Set the request
    //
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(*rtmsg));
    nlh->nlmsg_type = RTM_DELROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_ACK;
    nlh->nlmsg_seq = ns.seqno();
    nlh->nlmsg_pid = ns.nl_pid();
    rtmsg = static_cast<struct rtmsg*>(NLMSG_DATA(nlh));
    rtmsg->rtm_family = family;
    rtmsg->rtm_dst_len = fte.net().prefix_len(); // The destination mask length
    rtmsg->rtm_src_len = 0;
    rtmsg->rtm_tos = 0;
    rtmsg->rtm_table = RT_TABLE_MAIN;
    rtmsg->rtm_protocol = RTPROT_XORP;		// Mark this as a XORP route
    rtmsg->rtm_scope = RT_SCOPE_UNIVERSE;
    rtmsg->rtm_type = RTN_UNICAST;
    rtmsg->rtm_flags = RTM_F_NOTIFY;

    // Add the destination address as an attribute
    rta_len = RTA_LENGTH(fte.net().masked_addr().addr_bytelen());
    if (NLMSG_ALIGN(nlh->nlmsg_len) + rta_len > sizeof(buffer)) {
	XLOG_FATAL("AF_NETLINK buffer size error: %u instead of %u",
		   XORP_UINT_CAST(sizeof(buffer)),
		   XORP_UINT_CAST(NLMSG_ALIGN(nlh->nlmsg_len) + rta_len));
    }
    rtattr = RTM_RTA(rtmsg);
    rtattr->rta_type = RTA_DST;
    rtattr->rta_len = rta_len;
    data = static_cast<uint8_t*>(RTA_DATA(rtattr));
    fte.net().masked_addr().copy_out(data);
    nlh->nlmsg_len = NLMSG_ALIGN(nlh->nlmsg_len) + rta_len;

    do {
	//
	// When deleting a route which points to a discard interface,
	// pass the correct route type to the kernel.
	//
	if (fte.ifname().empty())
	    break;
	const IfTree& iftree = ftic().iftree();
	IfTree::IfMap::const_iterator ii = iftree.get_if(fte.ifname());
	//
	// XXX: unlike adding a route, we don't use XLOG_ASSERT()
	// to check whether the interface is configured in the system.
	// E.g., on startup we may attempt to clean-up leftover XORP
	// routes while we still don't have any interface tree configuration.
	//

	if ((ii != iftree.ifs().end()) && ii->second.discard())
	    rtmsg->rtm_type = RTN_BLACKHOLE;

	break;
    } while (false);

    int last_errno = 0;
    string error_msg;
    if (ns.sendto(&buffer, nlh->nlmsg_len, 0,
		  reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl))
	!= (ssize_t)nlh->nlmsg_len) {
	XLOG_ERROR("Error writing to netlink socket: %s", strerror(errno));
	return false;
    }
    if (NlmUtils::check_netlink_request(_ns_reader, ns, nlh->nlmsg_seq,
					last_errno, error_msg) < 0) {
	//
	// XXX: If the outgoing interface was taken down earlier, then
	// most likely the kernel has removed the matching forwarding
	// entries on its own. Hence, check whether all of the following
	// is true:
	//   - the error code matches
	//   - the outgoing interface is down
	//
	// If all conditions are true, then ignore the error and consider
	// the deletion was success.
	// Note that we could add to the following list the check whether
	// the forwarding entry is not in the kernel, but this is probably
	// an overkill. If such check should be performed, we should
	// use the corresponding FtiConfigTableGetNetlink provider.
	//
	do {
	    // Check whether the error code matches
	    if (last_errno != ESRCH) {
		//
		// XXX: The "No such process" error code is used by the
		// kernel to indicate there is no such forwarding entry
		// to delete.
		//
		break;
	    }

	    // Check whether the interface is down
	    if (fte.ifname().empty())
		break;		// No interface to check
	    const IfTree& iftree = ftic().iftree();
	    IfTree::IfMap::const_iterator ii = iftree.get_if(fte.ifname());
	    if ((ii == iftree.ifs().end()) || ii->second.enabled())
		break;		// The interface is UP

	    return (true);
	} while (false);

	XLOG_ERROR("Error checking netlink request: %s", error_msg.c_str());
	return false;
    }

    return true;
}

#endif // HAVE_NETLINK_SOCKETS
