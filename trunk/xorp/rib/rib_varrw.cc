// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/rib/rib_varrw.cc,v 1.4 2004/10/04 17:55:06 abittau Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "rib_varrw.hh"

template <class A>
RIBVarRW<A>::RIBVarRW(IPRouteEntry<A>& route)
    : _route(route)
{
}

template <class A>
void
RIBVarRW<A>::start_read()
{
    initialize("policytags", _route.policytags().element());

    read_route_nexthop(_route);

    ostringstream oss;

    oss << _route.metric();

    initialize("metric", _ef.create(ElemU32::id, oss.str().c_str()));
}

template <>
void
RIBVarRW<IPv4>::read_route_nexthop(IPRouteEntry<IPv4>& route)
{
    initialize("network4",
	       _ef.create(ElemIPv4Net::id, route.net().str().c_str()));
    initialize("nexthop4",
	       _ef.create(ElemIPv4::id, route.nexthop_addr().str().c_str()));
    initialize("network6", NULL);
    initialize("nexthop6", NULL);
}

template <>
void
RIBVarRW<IPv6>::read_route_nexthop(IPRouteEntry<IPv6>& route)
{
    initialize("network6",
	       _ef.create(ElemIPv6Net::id, route.net().str().c_str()));
    initialize("nexthop6",
	       _ef.create(ElemIPv6::id, route.nexthop_addr().str().c_str()));

    initialize("network4", NULL);
    initialize("nexthop4", NULL);
}

template <class A>
void
RIBVarRW<A>::single_write(const string& id, const Element& e)
{
    if (id == "policytags") {
	debug_msg("RIBVarRW writing policytags %s\n", e.str().c_str());
	_route.set_policytags(e);
    }
}


template class RIBVarRW<IPv4>;
template class RIBVarRW<IPv6>;
