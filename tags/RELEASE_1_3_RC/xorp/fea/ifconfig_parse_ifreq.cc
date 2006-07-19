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

#ident "$XORP: xorp/fea/ifconfig_parse_ifreq.cc,v 1.28 2006/04/26 01:42:15 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ether_compat.h"
#include "libxorp/ipvx.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_get.hh"
#include "ifconfig_media.hh"
#include "kernel_utils.hh"


//
// Parse information about network interface configuration change from
// the underlying system.
//
// The information to parse is in "struct ifreq" format
// (e.g., obtained by ioctl(SIOCGIFCONF) mechanism).
//

#ifndef HAVE_IOCTL_SIOCGIFCONF
bool
IfConfigGet::parse_buffer_ifreq(IfTree& , int ,
				const uint8_t* , size_t )
{
    return false;
}

#else // HAVE_IOCTL_SIOCGIFCONF

bool
IfConfigGet::parse_buffer_ifreq(IfTree& it, int family,
				const uint8_t* buf, size_t buf_bytes)
{
#ifndef HOST_OS_WINDOWS
    u_short if_index = 0;
    string if_name, alias_if_name;
    const uint8_t* ptr;
    
    for (ptr = buf; ptr < buf + buf_bytes; ) {
	bool is_newlink = false;	// True if really a new link
	size_t len = 0;
	const struct ifreq* ifreq = reinterpret_cast<const struct ifreq*>(ptr);
	struct ifreq ifrcopy;
	
	// Get the length of the ifreq entry
#ifdef HAVE_SA_LEN
	len = max(sizeof(struct sockaddr),
		  static_cast<size_t>(ifreq->ifr_addr.sa_len));
#else
	switch (ifreq->ifr_addr.sa_family) {
#ifdef HAVE_IPV6
	case AF_INET6:
	    len = sizeof(struct sockaddr_in6);
	    break;
#endif // HAVE_IPV6
	case AF_INET:
	default:
	    len = sizeof(struct sockaddr);
	    break;
	}
#endif // HAVE_SA_LEN
	len += sizeof(ifreq->ifr_name);
	len = max(len, sizeof(struct ifreq));
	ptr += len;				// Point to the next entry
	
	//
	// Get the interface name
	//
	char tmp_if_name[IFNAMSIZ+1];
	strncpy(tmp_if_name, ifreq->ifr_name, sizeof(tmp_if_name) - 1);
	tmp_if_name[sizeof(tmp_if_name) - 1] = '\0';
	char* cptr;
	if ( (cptr = strchr(tmp_if_name, ':')) != NULL) {
	    // Replace colon with null. Needed because in Solaris and Linux
	    // the interface name changes for aliases.
	    *cptr = '\0';
	}
	if_name = string(ifreq->ifr_name);
	alias_if_name = string(tmp_if_name);
	debug_msg("interface: %s\n", if_name.c_str());
	debug_msg("alias interface: %s\n", alias_if_name.c_str());
	
	//
	// Get the physical interface index
	//
	do {
	    if_index = 0;
	    
#ifdef HAVE_IF_NAMETOINDEX
	    if_index = if_nametoindex(if_name.c_str());
	    if (if_index > 0)
		break;
#endif // HAVE_IF_NAMETOINDEX
	    
#ifdef SIOCGIFINDEX
	    {
		struct ifreq ifridx;
		
		memset(&ifridx, 0, sizeof(ifridx));
		strncpy(ifridx.ifr_name, if_name.c_str(),
			sizeof(ifridx.ifr_name) - 1);
		if (ioctl(sock(family), SIOCGIFINDEX, &ifridx) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFINDEX) for interface %s failed: %s",
			       if_name.c_str(), strerror(errno));
		} else {
#ifdef HAVE_IFR_IFINDEX
		    if_index = ifridx.ifr_ifindex;
#else
		    if_index = ifridx.ifr_index;
#endif
		}
	    }
	    if (if_index > 0)
		break;
#endif // SIOCGIFINDEX
	    
	    break;
	} while (false);
	if (if_index == 0) {
	    XLOG_FATAL("Could not find physical interface index "
		       "for interface %s",
		       if_name.c_str());
	}
	debug_msg("interface index: %d\n", if_index);
	
	//
	// Add the interface (if a new one)
	//
	ifc().map_ifindex(if_index, alias_if_name);
	if (it.get_if(alias_if_name) == it.ifs().end()) {
	    it.add_if(alias_if_name);
	    is_newlink = true;
	}
	IfTreeInterface& fi = it.get_if(alias_if_name)->second;

	//
	// Set the physical interface index for the interface
	//
	if (is_newlink || (if_index != fi.pif_index()))
	    fi.set_pif_index(if_index);
	
	//
	// Get the MAC address
	//
	do {
#ifdef AF_LINK
	    const struct sockaddr* sa = &ifreq->ifr_addr;
	    if (sa->sa_family == AF_LINK) {
		const struct sockaddr_dl* sdl = reinterpret_cast<const struct sockaddr_dl*>(sa);
		if (sdl->sdl_type == IFT_ETHER) {
		    if (sdl->sdl_alen == sizeof(struct ether_addr)) {
			struct ether_addr ea;
			memcpy(&ea, sdl->sdl_data + sdl->sdl_nlen,
			       sdl->sdl_alen);
			EtherMac ether_mac(ea);
			if (is_newlink || (ether_mac != EtherMac(fi.mac())))
			    fi.set_mac(ether_mac);
			break;
		    } else if (sdl->sdl_alen != 0) {
			XLOG_ERROR("Address size %d uncatered for interface %s",
				   sdl->sdl_alen, if_name.c_str());
		    }
		}
	    }
#endif // AF_LINK
	    
#ifdef SIOCGIFHWADDR
	    memcpy(&ifrcopy, ifreq, sizeof(ifrcopy));
	    if (ioctl(sock(family), SIOCGIFHWADDR, &ifrcopy) < 0) {
		XLOG_ERROR("ioctl(SIOCGIFHWADDR) for interface %s failed: %s",
			   if_name.c_str(), strerror(errno));
	    } else {
		struct ether_addr ea;
		memcpy(&ea, ifrcopy.ifr_hwaddr.sa_data, sizeof(ea));
		EtherMac ether_mac(ea);
		if (is_newlink || (ether_mac != EtherMac(fi.mac())))
		    fi.set_mac(ether_mac);
		break;
	    }
#endif // SIOCGIFHWADDR
	    
	    break;
	} while (false);
	debug_msg("MAC address: %s\n", fi.mac().str().c_str());
	
	//
	// Get the MTU
	//
	unsigned int mtu = 0;
	memcpy(&ifrcopy, ifreq, sizeof(ifrcopy));
	if (ioctl(sock(family), SIOCGIFMTU, &ifrcopy) < 0) {
	    XLOG_ERROR("ioctl(SIOCGIFMTU) for interface %s failed: %s",
		       if_name.c_str(), strerror(errno));
	} else {
#ifndef HOST_OS_SOLARIS
	    mtu = ifrcopy.ifr_mtu;
#else
	    //
	    // XXX: Solaris supports ioctl(SIOCGIFMTU), but stores the MTU
	    // in the ifr_metric field instead of ifr_mtu.
	    //
	    mtu = ifrcopy.ifr_metric;
#endif // HOST_OS_SOLARIS
	}
	if (is_newlink || (mtu != fi.mtu()))
	    fi.set_mtu(mtu);
	debug_msg("MTU: %u\n", XORP_UINT_CAST(fi.mtu()));
	
	//
	// Get the flags
	//
	unsigned int flags = 0;
	memcpy(&ifrcopy, ifreq, sizeof(ifrcopy));
	if (ioctl(sock(family), SIOCGIFFLAGS, &ifrcopy) < 0) {
	    XLOG_ERROR("ioctl(SIOCGIFFLAGS) for interface %s failed: %s",
		       if_name.c_str(), strerror(errno));
	} else {
	    flags = ifrcopy.ifr_flags;
	}
	if (is_newlink || (flags != fi.if_flags())) {
	    fi.set_if_flags(flags);
	    fi.set_enabled(flags & IFF_UP);
	}
	debug_msg("enabled: %s\n", fi.enabled() ? "true" : "false");

	//
	// Get the link status
	//
	do {
	    bool no_carrier = false;
	    string error_msg;

	    if (ifconfig_media_get_link_status(if_name, no_carrier, error_msg)
		!= XORP_OK) {
		XLOG_ERROR("%s", error_msg.c_str());
		break;
	    }
	    if (is_newlink || (no_carrier != fi.no_carrier()))
		fi.set_no_carrier(no_carrier);
	    break;
	} while (false);
	debug_msg("no_carrier: %s\n", fi.no_carrier() ? "true" : "false");
	
	// XXX: vifname == ifname on this platform
	if (is_newlink)
	    fi.add_vif(alias_if_name);
	IfTreeVif& fv = fi.get_vif(alias_if_name)->second;
	
	//
	// Set the physical interface index for the vif
	//
	if (is_newlink || (if_index != fv.pif_index()))
	    fv.set_pif_index(if_index);
	
	//
	// Set the vif flags
	//
	if (is_newlink || (flags != fi.if_flags())) {
	    fv.set_enabled(fi.enabled() && (flags & IFF_UP));
	    fv.set_broadcast(flags & IFF_BROADCAST);
	    fv.set_loopback(flags & IFF_LOOPBACK);
#ifdef IFF_POINTOPOINT
	    fv.set_point_to_point(flags & IFF_POINTOPOINT);
#endif
	    fv.set_multicast(flags & IFF_MULTICAST);
	}
	debug_msg("vif enabled: %s\n", fv.enabled() ? "true" : "false");
	debug_msg("vif broadcast: %s\n", fv.broadcast() ? "true" : "false");
	debug_msg("vif loopback: %s\n", fv.loopback() ? "true" : "false");
	debug_msg("vif point_to_point: %s\n", fv.point_to_point() ? "true"
		  : "false");
	debug_msg("vif multicast: %s\n", fv.multicast() ? "true" : "false");
	
	//
	// Get the interface addresses for the same address family only.
	// XXX: if the address family is zero, then we query the address.
	//
	if ((ifreq->ifr_addr.sa_family != family)
	    && (ifreq->ifr_addr.sa_family != 0)) {
	    continue;
	}
	
	//
	// Get the IP address, netmask, broadcast address, P2P destination
	//
        // The default values
	IPvX lcl_addr = IPvX::ZERO(family);
	IPvX subnet_mask = IPvX::ZERO(family);
	IPvX broadcast_addr = IPvX::ZERO(family);
	IPvX peer_addr = IPvX::ZERO(family);
	bool has_broadcast_addr = false;
	bool has_peer_addr = false;
	
	struct ifreq ip_ifrcopy;
	memcpy(&ip_ifrcopy, ifreq, sizeof(ip_ifrcopy));
	ip_ifrcopy.ifr_addr.sa_family = family;
#ifdef SIOCGIFADDR_IN6
	struct in6_ifreq ip_ifrcopy6;
	memcpy(&ip_ifrcopy6, ifreq, sizeof(ip_ifrcopy6));
	ip_ifrcopy6.ifr_ifru.ifru_addr.sin6_family = family;
#endif // SIOCGIFADDR_IN6
	
	// Get the IP address
	if (ifreq->ifr_addr.sa_family == family) {
	    lcl_addr.copy_in(ifreq->ifr_addr);
	    memcpy(&ip_ifrcopy, ifreq, sizeof(ip_ifrcopy));
#ifdef SIOCGIFADDR_IN6
	    memcpy(&ip_ifrcopy6, ifreq, sizeof(ip_ifrcopy6));
#endif
	} else {
	    // XXX: we need to query the local IP address
	    XLOG_ASSERT(ifreq->ifr_addr.sa_family == 0);
	    
	    switch (family) {
	    case AF_INET:
#ifdef SIOCGIFADDR
		memset(&ifrcopy, 0, sizeof(ifrcopy));
		strncpy(ifrcopy.ifr_name, if_name.c_str(),
			sizeof(ifrcopy.ifr_name) - 1);
		ifrcopy.ifr_addr.sa_family = family;
		if (ioctl(sock(family), SIOCGIFADDR, &ifrcopy) < 0) {
		    // XXX: the interface probably has no address. Ignore.
		    continue;
		} else {
		    lcl_addr.copy_in(ifrcopy.ifr_addr);
		    memcpy(&ip_ifrcopy, &ifrcopy, sizeof(ip_ifrcopy));
		}
#endif // SIOCGIFADDR
		break;
		
#ifdef HAVE_IPV6
	    case AF_INET6:
#ifdef SIOCGIFADDR_IN6
	    {
		struct in6_ifreq ifrcopy6;
		
		memset(&ifrcopy6, 0, sizeof(ifrcopy6));
		strncpy(ifrcopy6.ifr_name, if_name.c_str(),
			sizeof(ifrcopy6.ifr_name) - 1);
		ifrcopy6.ifr_ifru.ifru_addr.sin6_family = family;
		if (ioctl(sock(family), SIOCGIFADDR_IN6, &ifrcopy6) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFADDR_IN6) failed: %s",
			       strerror(errno));
		} else {
		    lcl_addr.copy_in(ifrcopy6.ifr_ifru.ifru_addr);
		    memcpy(&ip_ifrcopy6, &ifrcopy6, sizeof(ip_ifrcopy6));
		}
	    }
#endif // SIOCGIFADDR_IN6
	    break;
#endif // HAVE_IPV6
	    
	    default:
		XLOG_UNREACHABLE();
		break;
	    }
	}
	lcl_addr = kernel_ipvx_adjust(lcl_addr);
	debug_msg("IP address: %s\n", lcl_addr.str().c_str());
	
	// Get the netmask
	switch (family) {
	case AF_INET:
#ifdef SIOCGIFNETMASK
	    memcpy(&ifrcopy, &ip_ifrcopy, sizeof(ifrcopy));
	    if (ioctl(sock(family), SIOCGIFNETMASK, &ifrcopy) < 0) {
		if (! fv.point_to_point()) {
		    XLOG_ERROR("ioctl(SIOCGIFNETMASK) failed: %s",
			       strerror(errno));
		}
	    } else {
		// The interface is configured
		// XXX: SIOCGIFNETMASK doesn't return proper family
		ifrcopy.ifr_addr.sa_family = family;
		subnet_mask.copy_in(ifrcopy.ifr_addr);
	    }
#endif // SIOCGIFNETMASK
	    break;
	    
#ifdef HAVE_IPV6
	case AF_INET6:
#ifdef SIOCGIFNETMASK_IN6
	{
	    struct in6_ifreq ifrcopy6;
	    
	    memcpy(&ifrcopy6, &ip_ifrcopy6, sizeof(ifrcopy6));
	    if (ioctl(sock(family), SIOCGIFNETMASK_IN6, &ifrcopy6) < 0) {
		if (! fv.point_to_point()) {
		    XLOG_ERROR("ioctl(SIOCGIFNETMASK_IN6) failed: %s",
			       strerror(errno));
		}
	    } else {
		// The interface is configured
		// XXX: SIOCGIFNETMASK_IN6 doesn't return proper family
		ifrcopy6.ifr_ifru.ifru_addr.sin6_family = family;
		subnet_mask.copy_in(ifrcopy6.ifr_ifru.ifru_addr);
	    }
	}
#endif // SIOCGIFNETMASK_IN6
	break;
#endif // HAVE_IPV6
	
	default:
	    XLOG_UNREACHABLE();
	    break;
	}
	debug_msg("IP netmask: %s\n", subnet_mask.str().c_str());
	
	// Get the broadcast address	
	if (fv.broadcast()) {
	    switch (family) {
	    case AF_INET:
#ifdef SIOCGIFBRDADDR
		memcpy(&ifrcopy, &ip_ifrcopy, sizeof(ifrcopy));
		if (ioctl(sock(family), SIOCGIFBRDADDR, &ifrcopy) < 0) {
		    XLOG_ERROR("ioctl(SIOCGIFBRADDR) failed: %s",
			       strerror(errno));
		} else {
		    // XXX: in case it doesn't return proper family
		    ifrcopy.ifr_broadaddr.sa_family = family;
		    broadcast_addr.copy_in(ifrcopy.ifr_addr);
		    has_broadcast_addr = true;
		}
#endif // SIOCGIFBRDADDR
		break;
		
#ifdef HAVE_IPV6
	    case AF_INET6:
		break;	// IPv6 doesn't have the idea of broadcast
#endif // HAVE_IPV6
		
	    default:
		XLOG_UNREACHABLE();
		break;
	    }
	    debug_msg("Broadcast address: %s\n", broadcast_addr.str().c_str());
	}
	
	// Get the p2p address
	if (fv.point_to_point()) {
	    switch (family) {
	    case AF_INET:
#ifdef SIOCGIFDSTADDR
		memcpy(&ifrcopy, &ip_ifrcopy, sizeof(ifrcopy));
		if (ioctl(sock(family), SIOCGIFDSTADDR, &ifrcopy) < 0) {
		    // Probably the p2p address is not configured
		} else {
		    // The p2p address is configured
		    // XXX: in case it doesn't return proper family
		    ifrcopy.ifr_dstaddr.sa_family = family;
		    peer_addr.copy_in(ifrcopy.ifr_dstaddr);
		    has_peer_addr = true;
		}
#endif // SIOCGIFDSTADDR
		break;
		
#ifdef HAVE_IPV6
	    case AF_INET6:
#ifdef SIOCGIFDSTADDR_IN6
	    {
		struct in6_ifreq ifrcopy6;
		
		memcpy(&ifrcopy6, &ip_ifrcopy6, sizeof(ifrcopy6));
		if (ioctl(sock(family), SIOCGIFDSTADDR_IN6, &ifrcopy6) < 0) {
		    // Probably the p2p address is not configured
		} else {
		    // The p2p address is configured
		    // Just in case it doesn't return proper family
		    ifrcopy6.ifr_ifru.ifru_dstaddr.sin6_family = family;
		    peer_addr.copy_in(ifrcopy6.ifr_ifru.ifru_dstaddr);
		    has_peer_addr = true;
		}
	    }
#endif // SIOCGIFDSTADDR_IN6
	    break;
#endif // HAVE_IPV6
	    
	    default:
		XLOG_UNREACHABLE();
		break;
	    }
	    debug_msg("Peer address: %s\n", peer_addr.str().c_str());
	}
	
	debug_msg("\n");	// put an empty line between interfaces
	
	// Add the address
	switch (family) {
	case AF_INET:
	{
	    fv.add_addr(lcl_addr.get_ipv4());
	    IfTreeAddr4& fa = fv.get_addr(lcl_addr.get_ipv4())->second;
	    fa.set_enabled(fv.enabled() && (flags & IFF_UP));
	    fa.set_broadcast(fv.broadcast()
			     && (flags & IFF_BROADCAST)
			     && has_broadcast_addr);
	    fa.set_loopback(fv.loopback() && (flags & IFF_LOOPBACK));
#ifdef IFF_POINTOPOINT
	    fa.set_point_to_point(fv.point_to_point()
				  && (flags & IFF_POINTOPOINT)
				  && has_peer_addr);
#else
	    UNUSED(has_peer_addr);
#endif
	    fa.set_multicast(fv.multicast() && (flags & IFF_MULTICAST));
	    
	    fa.set_prefix_len(subnet_mask.mask_len());
	    if (fa.broadcast())
		fa.set_bcast(broadcast_addr.get_ipv4());
	    if (fa.point_to_point())
		fa.set_endpoint(peer_addr.get_ipv4());
	    break;
	}
#ifdef HAVE_IPV6
	case AF_INET6:
	{
	    fv.add_addr(lcl_addr.get_ipv6());
	    IfTreeAddr6& fa = fv.get_addr(lcl_addr.get_ipv6())->second;
	    fa.set_enabled(fv.enabled() && (flags & IFF_UP));
	    fa.set_loopback(fv.loopback() && (flags & IFF_LOOPBACK));
	    fa.set_point_to_point(fv.point_to_point() && (flags & IFF_POINTOPOINT));
	    fa.set_multicast(fv.multicast() && (flags & IFF_MULTICAST));
	    
	    fa.set_prefix_len(subnet_mask.mask_len());
	    if (fa.point_to_point())
		fa.set_endpoint(peer_addr.get_ipv6());
	    break;
	}
#endif // HAVE_IPV6
	default:
	    XLOG_UNREACHABLE();
	    break;
	}
    }
    
    return true;
#else /* HOST_OS_WINDOWS */
    XLOG_FATAL("WinSock2 does not support struct ifreq.");

    UNUSED(it);
    UNUSED(family);
    UNUSED(buf);
    UNUSED(buf_bytes);
    return false;
#endif /* HOST_OS_WINDOWS */
}

#endif // HAVE_IOCTL_SIOCGIFCONF
