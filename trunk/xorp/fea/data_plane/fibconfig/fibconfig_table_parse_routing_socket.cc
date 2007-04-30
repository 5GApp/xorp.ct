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

#ident "$XORP: xorp/fea/forwarding_plane/fibconfig/fibconfig_table_parse_routing_socket.cc,v 1.5 2007/04/28 01:54:16 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HOST_OS_WINDOWS
#include "fea/win_rtsock.h"
#endif

#include "fea/fibconfig.hh"
#include "fea/fibconfig_table_get.hh"
#include "fea/routing_socket_utils.hh"


//
// Parse information about routing table information received from
// the underlying system.
//
// The information to parse is in RTM format
// (e.g., obtained by routing sockets or by sysctl(3) mechanism).
//
// Reading route(4) manual page is a good start for understanding this
//

#if !defined(HOST_OS_WINDOWS) && !defined(HAVE_ROUTING_SOCKETS)
bool
FibConfigTableGetSysctl::parse_buffer_routing_socket(int, const IfTree& ,
						     list<FteX>& ,
						     const vector<uint8_t>& ,
						     FibMsgSet)
{
    return false;
}

#else // HAVE_ROUTING_SOCKETS

bool
FibConfigTableGetSysctl::parse_buffer_routing_socket(int family,
						     const IfTree& iftree,
						     list<FteX>& fte_list,
						     const vector<uint8_t>& buffer,
						     FibMsgSet filter)
{
    AlignData<struct rt_msghdr> align_data(buffer);
    const struct rt_msghdr* rtm;
    size_t offset;

    rtm = align_data.payload();
    for (offset = 0; offset < buffer.size(); offset += rtm->rtm_msglen) {
	bool filter_match = false;

	rtm = align_data.payload_by_offset(offset);
	if (RTM_VERSION != rtm->rtm_version) {
	    XLOG_ERROR("RTM version mismatch: expected %d got %d",
		       RTM_VERSION,
		       rtm->rtm_version);
	    continue;
	}

	// XXX: ignore entries with an error
	if (rtm->rtm_errno != 0)
	    continue;

	if (filter & FibMsg::GETS) {
#ifdef RTM_GET
	    if ((rtm->rtm_type == RTM_GET) && (rtm->rtm_flags & RTF_UP))
		filter_match = true;
#endif
	}

	// Upcalls may not be supported in some BSD derived implementations.
	if (filter & FibMsg::RESOLVES) {
#ifdef RTM_MISS
	    if (rtm->rtm_type == RTM_MISS)
		filter_match = true;
#endif
#ifdef RTM_RESOLVE
	    if (rtm->rtm_type == RTM_RESOLVE)
		filter_match = true;
#endif
	}

	if (filter & FibMsg::UPDATES) {
	    if ((rtm->rtm_type == RTM_ADD) ||
		(rtm->rtm_type == RTM_DELETE) ||
		(rtm->rtm_type == RTM_CHANGE))
		    filter_match = true;
	}

	if (!filter_match)
	    continue;

#ifdef RTF_LLINFO
	if (rtm->rtm_flags & RTF_LLINFO)
	    continue;		// Ignore ARP table entries.
#endif
#ifdef RTF_WASCLONED
	if (rtm->rtm_flags & RTF_WASCLONED)
	    continue;		// XXX: ignore cloned entries
#endif
#ifdef RTF_CLONED
	if (rtm->rtm_flags & RTF_CLONED)
	    continue;		// XXX: ignore cloned entries
#endif
#ifdef RTF_MULTICAST
	if (rtm->rtm_flags & RTF_MULTICAST)
	    continue;		// XXX: ignore multicast entries
#endif
#ifdef RTF_BROADCAST
	if (rtm->rtm_flags & RTF_BROADCAST)
	    continue;		// XXX: ignore broadcast entries
#endif

	FteX fte(family);
	if (RtmUtils::rtm_get_to_fte_cfg(iftree, fte, rtm) == true) {
	    fte_list.push_back(fte);
	}
    }

    return true;
}

#endif // HAVE_ROUTING_SOCKETS
