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

#ident "$XORP: xorp/rip/redist.cc,v 1.6 2004/09/18 00:00:31 pavlin Exp $"

#include "rip_module.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"

#include "constants.hh"
#include "route_db.hh"
#include "route_entry.hh"
#include "redist.hh"

// ----------------------------------------------------------------------------
// RedistRouteOrigin

template <typename A>
uint32_t
RedistRouteOrigin<A>::expiry_secs() const
{
    return 0;
}

template <typename A>
uint32_t
RedistRouteOrigin<A>::deletion_secs() const
{
    return DEFAULT_DELETION_SECS;
}


// ----------------------------------------------------------------------------
// RouteRedistributor

template <typename A>
RouteRedistributor<A>::RouteRedistributor(RouteDB<A>&	rdb,
					  const string&	protocol,
					  uint16_t	cost,
					  uint16_t	tag)
    : _route_db(rdb), _protocol(protocol), _cost(cost), _tag(tag), _wdrawer(0)
{
    _rt_origin = new RedistRouteOrigin<A>();
}

template <typename A>
RouteRedistributor<A>::~RouteRedistributor()
{
    delete _rt_origin;
    delete _wdrawer;
}

template <typename A>
bool
RouteRedistributor<A>::add_route(const Net&  net, const Addr& nexthop,
				 const PolicyTags& policytags)
{
    _route_db.add_rib_route(net, nexthop, _cost, _tag, _rt_origin, policytags);
    return _route_db.update_route(net, nexthop, _cost, _tag, _rt_origin,
				  policytags);
}

template <typename A>
bool
RouteRedistributor<A>::add_route(const Net&  	net,
				 const Addr& 	nexthop,
				 uint16_t 	cost,
				 uint16_t 	tag,
				 const PolicyTags& policytags)
{
    _route_db.add_rib_route(net, nexthop, cost, tag, _rt_origin, policytags);
    return _route_db.update_route(net, nexthop, cost, tag, _rt_origin,
				  policytags);
}

template <typename A>
bool
RouteRedistributor<A>::expire_route(const Net& net)
{
    _route_db.delete_rib_route(net);
    return _route_db.update_route(net, A::ZERO(), RIP_INFINITY,
				  _tag, _rt_origin, PolicyTags());
}

template <typename A>
const string&
RouteRedistributor<A>::protocol() const
{
    return _protocol;
}

template <typename A>
uint16_t
RouteRedistributor<A>::tag() const
{
    return _tag;
}

template <typename A>
uint16_t
RouteRedistributor<A>::cost() const
{
    return _cost;
}

template <typename A>
uint32_t
RouteRedistributor<A>::route_count() const
{
    return _rt_origin->route_count();
}

template <typename A>
void
RouteRedistributor<A>::withdraw_routes()
{
    if (_wtimer.scheduled() == false) {
	EventLoop& e = _route_db.eventloop();
	_wtimer = e.new_periodic(5,
			callback(this, &RouteRedistributor::withdraw_batch));
    }
}

template <typename A>
bool
RouteRedistributor<A>::withdrawing_routes() const
{
    return _wtimer.scheduled();
}

template <typename A>
bool
RouteRedistributor<A>::withdraw_batch()
{
    if (_wdrawer == 0) {
	_wdrawer = new RouteWalker<A>(_route_db);
	_wdrawer->reset();
    }

    XLOG_ASSERT(_wdrawer->state() == RouteWalker<A>::STATE_RUNNING);

    uint32_t visited = 0;
    const RouteEntry<A>* r = _wdrawer->current_route();
    while (r != 0) {
	if (r->origin() == _rt_origin) {
	    _route_db.update_route(r->net(), r->nexthop(),
				   RIP_INFINITY, r->tag(),
				   _rt_origin, r->policytags());
	}
	r = _wdrawer->next_route();

	if (++visited == 5) {
	    return true;	// we're not finished - reschedule timer
	}
    }
    delete _wdrawer;
    _wdrawer = 0;
    return false; // we're finished - cancel timer
}


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class RedistRouteOrigin<IPv4>;
template class RouteRedistributor<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class RouteRedistributor<IPv6>;
template class RedistRouteOrigin<IPv6>;
#endif
