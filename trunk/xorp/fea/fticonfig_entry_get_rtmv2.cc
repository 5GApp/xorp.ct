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

#ident "$XORP: xorp/fea/fticonfig_entry_get_rtmv2.cc,v 1.4 2006/08/29 22:42:18 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvxnet.hh"

#ifdef HOST_OS_WINDOWS
#include "win_rtsock.h"
#endif

#include "fticonfig.hh"
#include "fticonfig_entry_get.hh"

//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is Router Manager V2.
//


FtiConfigEntryGetRtmV2::FtiConfigEntryGetRtmV2(FtiConfig& ftic)
    : FtiConfigEntryGet(ftic)
{
#ifdef HOST_OS_WINDOWS
    register_ftic_primary();
#endif
}

FtiConfigEntryGetRtmV2::~FtiConfigEntryGetRtmV2()
{
#if 0
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Router Manager V2 mechanism to get "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
#endif
}

int
FtiConfigEntryGetRtmV2::start(string& error_msg)
{
#if 0
    if (_is_running)
	return (XORP_OK);

    if (WinRtmPipe::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

#endif
    return (XORP_OK);
    UNUSED(error_msg);
}

int
FtiConfigEntryGetRtmV2::stop(string& error_msg)
{
#if 0
    if (! _is_running)
	return (XORP_OK);

    if (WinRtmPipe::stop(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = false;
#endif

    return (XORP_OK);
    UNUSED(error_msg);
}

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetRtmV2::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = ftex.get_fte4();
    
    return (ret_value);
}

/**
 * Lookup a route by network address.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetRtmV2::lookup_route_by_network4(const IPv4Net& dst,
						  Fte4& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_network(IPvXNet(dst), ftex);
    
    fte = ftex.get_fte4();
    
    return (ret_value);
}

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetRtmV2::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = ftex.get_fte6();
    
    return (ret_value);
}

/**
 * Lookup route by network address.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetRtmV2::lookup_route_by_network6(const IPv6Net& dst,
						  Fte6& fte)
{ 
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_network(IPvXNet(dst), ftex);
    
    fte = ftex.get_fte6();
    
    return (ret_value);
}

#ifndef HOST_OS_WINDOWS
bool
FtiConfigEntryGetRtmV2::lookup_route_by_dest(const IPvX& , FteX& )
{
    return false;
}

bool
FtiConfigEntryGetRtmV2::lookup_route_by_network(const IPvXNet& , FteX& )
{
    return false;
}

#else // HOST_OS_WINDOWS

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetRtmV2::lookup_route_by_dest(const IPvX& dst, FteX& fte)
{
#if 1
    /*
     * XXX: The RTM_GET stuff isn't supported on Windows yet.
     */
    return (false);
    UNUSED(dst);
    UNUSED(fte);
#else
    static const size_t	buffer_size = sizeof(struct rt_msghdr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct rt_msghdr rtm;
    } buffer;
    struct rt_msghdr*	rtm = &buffer.rtm;
    struct sockaddr_in*	sin;
    WinRtmPipe&		rs = *this;
    
    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! ftic().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);
    
    // Check that the destination address is valid
    if (! dst.is_unicast()) {
	return false;
    }
    
    //
    // Set the request
    //
    memset(&buffer, 0, sizeof(buffer));
    switch (dst.af()) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + sizeof(struct sockaddr_in6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_GET;
    rtm->rtm_addrs = (RTA_DST | RTA_IFP);
    rtm->rtm_flags = RTF_UP;
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();
    
    // Copy the destination address
    sin = reinterpret_cast<struct sockaddr_in*>(rtm + 1);
    dst.copy_out(*sin);

    //
    // Add extra space for sockaddr_dl that corresponds to the RTA_IFP flag.
    // Required if the OS is very strict in the arguments checking
    // (e.g., NetBSD).
    //
#ifdef AF_LINK
    do {
	// Set the data-link socket
	struct sockaddr_dl* sdl;

	rtm->rtm_msglen += sizeof(struct sockaddr_dl);
	switch (dst.af()) {
	case AF_INET:
	    sdl = ADD_POINTER(sin, sizeof(struct sockaddr_in),
			      struct sockaddr_dl*);
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
	    sdl = ADD_POINTER(sin, sizeof(struct sockaddr_in6),
			      struct sockaddr_dl*);
	    break;
#endif // HAVE_IPV6
	default:
	    XLOG_UNREACHABLE();
	    break;
	}
	sdl->sdl_family = AF_LINK;
#ifdef HAVE_SDL_LEN
	sdl->sdl_len = sizeof(struct sockaddr_dl);
#endif
    } while (false);
#endif // AF_LINK

    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("Error writing to Rtmv2 pipe: %s", strerror(errno));
	return false;
    }

    //
    // Force to receive data from the kernel, and then parse it
    //
    string error_msg;
    if (_rs_reader.receive_data(rs, rtm->rtm_seq, error_msg) != XORP_OK) {
	XLOG_ERROR("Error reading from Rtmv2 pipe: %s", error_msg.c_str());
	return (false);
    }
    if (parse_buffer_rtm(fte, _rs_reader.buffer(), FtiFibMsg::GETS) != true) {
	return (false);
    }

    return (true);
#endif
}

/**
 * Lookup route by network.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetRtmV2::lookup_route_by_network(const IPvXNet& dst, FteX& fte)
{
#if 1
    /*
     * XXX: The RTM_GET stuff isn't supported on Windows yet.
     */
    return (false);
    UNUSED(dst);
    UNUSED(fte);
#else
    static const size_t	buffer_size = sizeof(struct rt_msghdr) + 512;
    union {
	uint8_t		data[buffer_size];
	struct rt_msghdr rtm;
    } buffer;
    struct rt_msghdr*	rtm = &buffer.rtm;
    struct sockaddr_in*	sin;
    WinRtmPipe&		rs = *this;
    
    // Zero the return information
    fte.zero();

    // Check that the family is supported
    do {
	if (dst.is_ipv4()) {
	    if (! ftic().have_ipv4())
		return false;
	    break;
	}
	if (dst.is_ipv6()) {
	    if (! ftic().have_ipv6())
		return false;
	    break;
	}
	break;
    } while (false);

    // Check that the destination address is valid    
    if (! dst.is_unicast()) {
	return false;
    }

    //
    // Set the request
    //
    memset(&buffer, 0, sizeof(buffer));
    switch (dst.af()) {
    case AF_INET:
	rtm->rtm_msglen = sizeof(*rtm) + 2 * sizeof(struct sockaddr_in);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	rtm->rtm_msglen = sizeof(*rtm) + 2 * sizeof(struct sockaddr_in6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    rtm->rtm_version = RTM_VERSION;
    rtm->rtm_type = RTM_GET;
    rtm->rtm_addrs = (RTA_DST | RTA_NETMASK | RTA_IFP);
    rtm->rtm_flags = RTF_UP;
    rtm->rtm_pid = rs.pid();
    rtm->rtm_seq = rs.seqno();

    // Copy the destination address    
    sin = reinterpret_cast<struct sockaddr_in*>(rtm + 1);
    dst.masked_addr().copy_out(*sin);
    
    // Copy the network mask
    switch (dst.af()) {
    case AF_INET:
	sin = ADD_POINTER(sin, sizeof(struct sockaddr_in),
			  struct sockaddr_in*);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	sin = ADD_POINTER(sin, sizeof(struct sockaddr_in6),
			  struct sockaddr_in*);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    IPvX netmask = IPvX::make_prefix(dst.af(), dst.prefix_len());
    netmask.copy_out(*sin);

    //
    // Add extra space for sockaddr_dl that corresponds to the RTA_IFP flag.
    // Required if the OS is very strict in the arguments checking
    // (e.g., NetBSD).
    //
#ifdef AF_LINK
    do {
	// Set the data-link socket
	struct sockaddr_dl* sdl;

	rtm->rtm_msglen += sizeof(struct sockaddr_dl);
	switch (dst.af()) {
	case AF_INET:
	    sdl = ADD_POINTER(sin, sizeof(struct sockaddr_in),
			      struct sockaddr_dl*);
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
	    sdl = ADD_POINTER(sin, sizeof(struct sockaddr_in6),
			      struct sockaddr_dl*);
	    break;
#endif // HAVE_IPV6
	default:
	    XLOG_UNREACHABLE();
	    break;
	}
	sdl->sdl_family = AF_LINK;
#ifdef HAVE_SDL_LEN
	sdl->sdl_len = sizeof(struct sockaddr_dl);
#endif
    } while (false);
#endif // AF_LINK

    if (rs.write(rtm, rtm->rtm_msglen) != rtm->rtm_msglen) {
	XLOG_ERROR("Error writing to Rtmv2 pipe: %s", strerror(errno));
	return false;
    }
    
    //
    // Force to receive data from the kernel, and then parse it
    //
    string error_msg;
    if (_rs_reader.receive_data(rs, rtm->rtm_seq, error_msg) != XORP_OK) {
	XLOG_ERROR("Error reading from Rtmv2 pipe: %s", error_msg.c_str());
	return (false);
    }

    if (parse_buffer_rtm(fte, _rs_reader.buffer(), FtiFibMsg::GETS) != true) {
	return (false);
    }

    return (true);
#endif // 0
}

#endif // HOST_OS_WINDOWS
