// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/rib/rt_tab_log.cc,v 1.13 2008/03/10 20:15:33 pavlin Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "rt_tab_log.hh"


template<typename A>
LogTable<A>::LogTable(const string&   tablename,
		      RouteTable<A>*  parent)
    : RouteTable<A>(tablename), _update_number(0)
{
    _parent = parent;
    _parent->set_next_table(this);
}

template<typename A>
LogTable<A>::~LogTable()
{
}

template<typename A>
uint32_t 
LogTable<A>::update_number() const 
{ 
    return _update_number; 
}

template<typename A>
int
LogTable<A>::add_route(const IPRouteEntry<A>& 	route,
		       RouteTable<A>* 		caller)
{
    _update_number++;
    RouteTable<A>* n = this->next_table();
    if (n != NULL) {
	return n->add_route(route, caller);
    }
    return XORP_OK;
}

template<typename A>
int
LogTable<A>::delete_route(const IPRouteEntry<A>* route,
			  RouteTable<A>* 	 caller)
{
    RouteTable<A>* n = this->next_table();
    if (n != NULL) {
	return n->delete_route(route, caller);
    }
    _update_number++;
    return XORP_OK;
}

template<typename A>
const IPRouteEntry<A>*
LogTable<A>::lookup_route(const IPNet<A>& net) const
{
    return _parent->lookup_route(net);
}

template<typename A>
const IPRouteEntry<A>*
LogTable<A>::lookup_route(const A& addr) const
{
    return _parent->lookup_route(addr);
}

template<typename A>
void
LogTable<A>::replumb(RouteTable<A>* old_parent,
		     RouteTable<A>* new_parent)
{
    XLOG_ASSERT(_parent == old_parent);
    _parent = new_parent;

    // XXX _parent->set_next_table???
}

template<typename A>
RouteRange<A>*
LogTable<A>::lookup_route_range(const A& addr) const
{
    return _parent->lookup_route_range(addr);
}

template<typename A> string
LogTable<A>::str() const
{
    string s;
    s = "-------\nLogTable: " + this->tablename() + "\n";
    s += "parent = " + _parent->tablename() + "\n";
    return s;
}


// ----------------------------------------------------------------------------

template <typename A>
OstreamLogTable<A>::OstreamLogTable(const string&	tablename,
				    RouteTable<A>*	parent,
				    std::ostream&	out)
    : LogTable<A>(tablename, parent), _o(out)
{
}

template <typename A>
int
OstreamLogTable<A>::add_route(const IPRouteEntry<A>& 	route,
			      RouteTable<A>* 		caller)
{
    _o << this->update_number() << " Add: " << route.str() << " Return: ";
    int s = LogTable<A>::add_route(route, caller);
    _o << s << std::endl;
    return s;
}

template <typename A>
int
OstreamLogTable<A>::delete_route(const IPRouteEntry<A>* 	route,
				 RouteTable<A>* 		caller)
{
    if (route != NULL) {
	_o << this->update_number() << " Delete: " << route->str() << " Return: ";
    }

    int s = LogTable<A>::delete_route(route, caller);

    if (route != NULL) {
	_o << s << std::endl;
    }
    return s;
}

template <typename A>
string
OstreamLogTable<A>::str() const
{
    return "OstreamLogTable<" + A::ip_version_str() + ">";
}

template class OstreamLogTable<IPv4>;
template class OstreamLogTable<IPv6>;


// ----------------------------------------------------------------------------
#include "rib_module.h"
#include "libxorp/xlog.h"

template <typename A>
XLogTraceTable<A>::XLogTraceTable(const string&	tablename,
				  RouteTable<A>* parent)

    : LogTable<A>(tablename, parent)
{
}

template <typename A>
int
XLogTraceTable<A>::add_route(const IPRouteEntry<A>& 	route,
			      RouteTable<A>* 		caller)
{
    string msg = c_format("%u Add: %s Return: ",
			  XORP_UINT_CAST(this->update_number()),
			  route.str().c_str());
    int s = LogTable<A>::add_route(route, caller);
    msg += c_format("%d\n", s);
    XLOG_TRACE(true, "%s", msg.c_str());

    return s;
}

template <typename A>
int
XLogTraceTable<A>::delete_route(const IPRouteEntry<A>* 	route,
				RouteTable<A>* 		caller)
{
    string msg;

    if (route != NULL) {
	msg = c_format("%u Delete: %s Return: ",
		       XORP_UINT_CAST(this->update_number()),
		       route->str().c_str());
    }

    int s = LogTable<A>::delete_route(route, caller);

    if (route != NULL) {
	msg += c_format("%d\n", s);
	XLOG_TRACE(true, "%s", msg.c_str());
    }
    return s;
}

template <typename A>
string
XLogTraceTable<A>::str() const
{
    return "XLogTraceTable<" + A::ip_version_str() + ">";
}

template class XLogTraceTable<IPv4>;
template class XLogTraceTable<IPv6>;


// ----------------------------------------------------------------------------

#define DEBUG_LOGGING
#include "libxorp/debug.h"

template <typename A>
DebugMsgLogTable<A>::DebugMsgLogTable(const string&	tablename,
				  RouteTable<A>* parent)

    : LogTable<A>(tablename, parent)
{
}

template <typename A>
int
DebugMsgLogTable<A>::add_route(const IPRouteEntry<A>& 	route,
			      RouteTable<A>* 		caller)
{
    string msg = c_format("%u Add: %s Return: ",
			  XORP_UINT_CAST(this->update_number()),
			  route.str().c_str());
    int s = LogTable<A>::add_route(route, caller);
    msg += c_format("%d\n", s);
    debug_msg("%s", msg.c_str());

    return s;
}

template <typename A>
int
DebugMsgLogTable<A>::delete_route(const IPRouteEntry<A>* 	route,
				RouteTable<A>* 		caller)
{
    string msg;

    if (route != NULL) {
	msg = c_format("%u Delete: %s Return: ",
		       XORP_UINT_CAST(this->update_number()),
		       route->str().c_str());
    }

    int s = LogTable<A>::delete_route(route, caller);

    if (route != NULL) {
	msg += c_format("%d\n", s);
	debug_msg("%s", msg.c_str());
    }
    return s;
}

template <typename A>
string
DebugMsgLogTable<A>::str() const
{
    return "DebugMsgLogTable<" + A::ip_version_str() + ">";
}

template class DebugMsgLogTable<IPv4>;
template class DebugMsgLogTable<IPv6>;
