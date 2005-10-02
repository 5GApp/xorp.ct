// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/static_routes/static_routes_varrw.cc,v 1.6 2005/09/04 18:35:50 abittau Exp $"

#include "static_routes_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "static_routes_varrw.hh"

StaticRoutesVarRW::StaticRoutesVarRW(StaticRoute& route)
    : _route(route), _is_ipv4(route.is_ipv4()), _is_ipv6(route.is_ipv6())
{
}

void
StaticRoutesVarRW::start_read()
{
    initialize(VAR_POLICYTAGS, _route.policytags().element());

    if (_is_ipv4) {
	initialize(VAR_NETWORK4,
		   _ef.create(ElemIPv4Net::id,
			      _route.network().str().c_str()));
	initialize(VAR_NEXTHOP4,
		   _ef.create(ElemIPv4::id,
			      _route.nexthop().str().c_str()));
	
	initialize(VAR_NETWORK6, NULL);
	initialize(VAR_NEXTHOP6, NULL);
    }

    if (_is_ipv6) {
	initialize(VAR_NETWORK6,
		   _ef.create(ElemIPv6Net::id,
			      _route.network().str().c_str()));
	initialize(VAR_NEXTHOP6,
		   _ef.create(ElemIPv6::id,
			      _route.nexthop().str().c_str()));

	initialize(VAR_NETWORK4, NULL);
	initialize(VAR_NEXTHOP4, NULL);
    }

    ostringstream oss;

    oss << _route.metric();

    initialize(VAR_METRIC, _ef.create(ElemU32::id, oss.str().c_str()));
}

void
StaticRoutesVarRW::single_write(const Id& id, const Element& e)
{
    if (id == VAR_POLICYTAGS) {
	_route.set_policytags(e);
    }
}

Element*
StaticRoutesVarRW::single_read(const Id& /* id */)
{
    XLOG_UNREACHABLE();
}
