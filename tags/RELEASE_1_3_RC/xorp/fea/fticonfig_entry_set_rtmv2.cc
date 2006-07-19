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

#ident "$XORP: xorp/fea/fticonfig_entry_set_rtsock.cc,v 1.35 2006/04/03 04:31:21 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HOST_OS_WINDOWS
#include "win_rtsock.h"
#endif
#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif
#ifdef HAVE_ROUTPROT_H
#include <routprot.h>
#endif

#include "fticonfig.hh"
#include "fticonfig_entry_set.hh"
#include "iftree.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is Router Manager V2.
//


FtiConfigEntrySetRtmV2::FtiConfigEntrySetRtmV2(FtiConfig& ftic)
    : FtiConfigEntrySet(ftic),
      _rs4(NULL),
      _rs6(NULL)
{
#ifdef HOST_OS_WINDOWS
    if (!WinSupport::is_rras_running()) {
	XLOG_WARNING("RRAS is not running; disabling FtiConfigEntrySetRtmV2.");
        return;
    }

    _rs4 = new WinRtmPipe(ftic.eventloop());

#ifdef HAVE_IPV6
    _rs6 = new WinRtmPipe(ftic.eventloop());
#endif

    register_ftic_primary();
#endif
}

FtiConfigEntrySetRtmV2::~FtiConfigEntrySetRtmV2()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Router Manager V2 mechanism to set "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
#ifdef HAVE_IPV6
    if (_rs6)
        delete _rs6;
#endif
    if (_rs4)
        delete _rs4;
}

int
FtiConfigEntrySetRtmV2::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    if (_rs4 == NULL || _rs4->start(AF_INET, error_msg) < 0)
        return (XORP_ERROR);
#if 0
#ifdef HAVE_IPV6
    if (_rs6 == NULL || _rs6->start(AF_INET6, error_msg) < 0)
        return (XORP_ERROR);
#endif
#endif
    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigEntrySetRtmV2::stop(string& error_msg)
{
    int result;

    if (! _is_running)
	return (XORP_OK);

    if (_rs4 == NULL || _rs4->stop(error_msg) < 0)
        result = XORP_ERROR;
#if 0
#ifdef HAVE_IPV6
    if (rs6 == NULL || _rs6->stop(error_msg) < 0)
        result = XORP_ERROR;
#endif
#endif

    _is_running = false;

    return (XORP_OK);
}

bool
FtiConfigEntrySetRtmV2::add_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetRtmV2::delete_entry4(const Fte4& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

bool
FtiConfigEntrySetRtmV2::add_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (add_entry(ftex));
}

bool
FtiConfigEntrySetRtmV2::delete_entry6(const Fte6& fte)
{
    FteX ftex(fte);
    
    return (delete_entry(ftex));
}

#ifndef HOST_OS_WINDOWS
bool
FtiConfigEntrySetRtmV2::add_entry(const FteX& )
{
    return false;
}

bool
FtiConfigEntrySetRtmV2::delete_entry(const FteX& )
{
    return false;
}

#else // HOST_OS_WINDOWS

bool
FtiConfigEntrySetRtmV2::add_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rt_msghdr) + (sizeof(struct sockaddr_storage) * 3);
    char		buffer[buffer_size];
    struct rt_msghdr	*rtm;
    struct sockaddr_storage *ss;
    struct sockaddr_in	*sin_dst, *sin_netmask;
    struct sockaddr_in	*sin_nexthop = NULL;
    int			family = fte.net().af();
    IPvX		fte_nexthop = fte.nexthop();
    int			result;

    debug_msg("add_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte_nexthop.str().c_str());

    // Check that the family is supported
    do {
	if (fte_nexthop.is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (fte_nexthop.is_ipv6()) {
	    if (! ftic().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);
    
    if (fte.is_connected_route())
	return true;	// XXX: don't add/remove directly-connected routes

#if 0
    if (! fte.ifname().empty())
	is_interface_route = true;
#endif

#if 0
    do {
	//
	// Check for a discard route; the referenced ifname must have the
	// discard property. The next-hop is forcibly rewritten to be the
	// loopback address, in order for the RTF_BLACKHOLE flag to take
	// effect on BSD platforms.
	//
	if (fte.ifname().empty())
	    break;
	const IfTree& iftree = ftic().iftree();
	IfTree::IfMap::const_iterator ii = iftree.get_if(fte.ifname());
	if (ii == iftree.ifs().end()) {
	    XLOG_ERROR("Invalid interface name: %s", fte.ifname().c_str());
	    return false;
	}
	if (ii->second.discard()) {
	    is_discard_route = true;
	    fte_nexthop = IPvX::LOOPBACK(family);
	}
	break;
    } while (false);
#endif

    //
    // Set the request
    //
    memset(buffer, 0, sizeof(buffer));
    rtm = reinterpret_cast<struct rt_msghdr *>(buffer);
    rtm->rtm_msglen = sizeof(*rtm);
    
    if (family != AF_INET && family != AF_INET6)
	XLOG_UNREACHABLE();

    ss = (struct sockaddr_storage *)(rtm + 1);
    sin_dst = (struct sockaddr_in *)(ss);
    sin_nexthop = (struct sockaddr_in *)(ss + 1);
    sin_netmask = (struct sockaddr_in *)(ss + 2);
    rtm->rtm_msglen += (3 * sizeof(struct sockaddr_storage));

    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_ADD;
    rtm->rtm_addrs = (RTA_DST | RTA_GATEWAY | RTA_NETMASK);
    rtm->rtm_pid = ((family == AF_INET ? _rs4 : _rs6))->pid();
    rtm->rtm_seq = ((family == AF_INET ? _rs4 : _rs6))->seqno();

    // Copy the interface index.
    const IfTree& iftree = ftic().iftree();
    IfTree::IfMap::const_iterator ii = iftree.get_if(fte.ifname());
    XLOG_ASSERT(ii != iftree.ifs().end());
    rtm->rtm_index = ii->second.pif_index();

    // Copy the destination, the nexthop, and the netmask addresses
    fte.net().masked_addr().copy_out(*sin_dst);
    if (sin_nexthop != NULL) {
	fte_nexthop.copy_out(*sin_nexthop);
    }
    fte.net().netmask().copy_out(*sin_netmask);

    result = ((family == AF_INET ? _rs4 : _rs6))->write(rtm, rtm->rtm_msglen);
    if (result != rtm->rtm_msglen) {
	XLOG_ERROR("Error writing to Rtmv2 pipe: %s", strerror(errno));
	return false;
    }
    
    //
    // TODO: here we should check the routing socket output whether the write
    // succeeded.
    //
    
    return true;
}

bool
FtiConfigEntrySetRtmV2::delete_entry(const FteX& fte)
{
    static const size_t	buffer_size = sizeof(struct rt_msghdr) + (sizeof(struct sockaddr_storage) * 2);
    char		buffer[buffer_size];
    struct rt_msghdr	*rtm;
    struct sockaddr_storage *ss;
    struct sockaddr_in	*sin_dst, *sin_netmask = NULL;
    int			family = fte.net().af();
    int			result;

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
    
    //
    // Set the request
    //
    memset(buffer, 0, sizeof(buffer));
    rtm = reinterpret_cast<struct rt_msghdr *>(buffer);
    rtm->rtm_msglen = sizeof(*rtm);

    if (family != AF_INET && family != AF_INET6)
	XLOG_UNREACHABLE();
    
    ss = (struct sockaddr_storage *)(rtm + 1);
    sin_dst = (struct sockaddr_in *)(ss);
    sin_netmask = (struct sockaddr_in *)(ss + 1);
    rtm->rtm_msglen += (2 * sizeof(struct sockaddr_storage));

    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_DELETE;
    rtm->rtm_addrs = RTA_DST;
    rtm->rtm_addrs |= RTA_NETMASK;
    rtm->rtm_flags = 0;
    rtm->rtm_pid = ((family == AF_INET ? _rs4 : _rs6))->pid();
    rtm->rtm_seq = ((family == AF_INET ? _rs4 : _rs6))->seqno();

    // Copy the interface index.
    const IfTree& iftree = ftic().iftree();
    IfTree::IfMap::const_iterator ii = iftree.get_if(fte.ifname());
    XLOG_ASSERT(ii != iftree.ifs().end());
    rtm->rtm_index = ii->second.pif_index();
    
    // Copy the destination, and the netmask addresses (if needed)
    fte.net().masked_addr().copy_out(*sin_dst);
    fte.net().netmask().copy_out(*sin_netmask);
    
    result = ((family == AF_INET ? _rs4 : _rs6))->write(rtm, rtm->rtm_msglen);
    if (result != rtm->rtm_msglen) {
	XLOG_ERROR("Error writing to Rtmv2 pipe: %s", strerror(errno));
	return false;
    }
    
    //
    // TODO: here we should check the routing socket output whether the write
    // succeeded.
    //
    
    return true;
}
#endif // HOST_OS_WINDOWS
