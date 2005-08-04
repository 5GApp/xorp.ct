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

#ident "$XORP: xorp/bgp/route_table_policy.cc,v 1.11 2005/07/27 19:12:59 abittau Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"

#include "route_table_policy.hh"
#include "bgp_varrw.hh"
#include "route_table_decision.hh"
#include "route_table_ribin.hh"


template <class A>
PolicyTable<A>::PolicyTable(const string& tablename, const Safi& safi,
			    BGPRouteTable<A>* parent,
			    PolicyFilters& pfs, 
			    const filter::Filter& type)
    : BGPRouteTable<A>(tablename, safi), _filter_type(type),
      _policy_filters(pfs)
{
    this->_parent = parent;		
}

template <class A>
PolicyTable<A>::~PolicyTable()
{
}

template <class A>
const InternalMessage<A>*
PolicyTable<A>::do_filtering(const InternalMessage<A>& rtmsg, 
			     bool no_modify) const
{
    BGPVarRW<A>* varrw = get_varrw(rtmsg, no_modify);

    XLOG_ASSERT(varrw);
    try {
	bool accepted = true;

	void* pf = NULL;
	int pfi = 0;
	switch (_filter_type) {
	    case filter::IMPORT:
		pfi = 0;
		break;

	    case filter::EXPORT_SOURCEMATCH:
		pfi = 1;
		break;
	
	    case filter::EXPORT:
		pfi = 2;
		break;
	}
	pf = rtmsg.route()->policyfilter(pfi).get();
	debug_msg("[BGP] running filter %s on route: %s (filter=%p)\n",
		  filter::filter2str(_filter_type).c_str(),
		  rtmsg.str().c_str(), pf);

	accepted = _policy_filters.run_filter(_filter_type, *varrw);

	pf = rtmsg.route()->policyfilter(pfi).get();
	debug_msg("[BGP] filter after filtering=%p\n", pf);

	// we just did a filtering, so a filter must be assigned to this route!
	if (!no_modify) {
	    XLOG_ASSERT(pf);
	}

	if (!accepted) {
	    delete varrw;
	    return NULL;
	}

	// we only want to check if filter accepted / rejecd route
	// [for route lookups]
	if (no_modify) {
	    delete varrw;
	    return &rtmsg;
	}    

	if (!varrw->modified()) {
	    delete varrw;
	    return &rtmsg;
	}    


	InternalMessage<A>* fmsg = varrw->filtered_message();

	debug_msg("[BGP] filter modified message: %s\n", fmsg->str().c_str());

	delete varrw;
	return fmsg;
    } catch(const PolicyException& e) {
	XLOG_FATAL("Policy filter error %s", e.str().c_str());
	XLOG_UNFINISHED();
	delete varrw;
    }
}

template <class A>
BGPVarRW<A>*
PolicyTable<A>::get_varrw(const InternalMessage<A>& rtmsg, bool no_modify) const
{
    return new BGPVarRW<A>(rtmsg, no_modify, filter::filter2str(_filter_type));
}

template <class A>
int
PolicyTable<A>::add_route(const InternalMessage<A> &rtmsg,
			  BGPRouteTable<A> *caller)
{
    XLOG_ASSERT(caller == this->_parent);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    debug_msg("[BGP] PolicyTable %s add_route: %s\n",
	      filter::filter2str(_filter_type).c_str(),
	      rtmsg.str().c_str());

#if 0
    // if we are getting an add_route, it must? be a new route... Thus the
    // policy filter should be null
    switch (_filter_type) {
	case filter::IMPORT:
	   XLOG_ASSERT(rtmsg.route()->policyfilter(0).is_empty());
	   break;

	case filter::EXPORT_SOURCEMATCH:
	   XLOG_ASSERT(rtmsg.route()->policyfilter(1).is_empty());
	   break;

	// when a new peering comes up, the routes will be sent as an ADD!
	case filter::EXPORT:
//	   XLOG_ASSERT(rtmsg.route()->policyfilter(2).is_empty());
	   break;
    }
#endif

    const InternalMessage<A>* fmsg = do_filtering(rtmsg, false);

    if (rtmsg.changed() && fmsg != &rtmsg) {
	debug_msg("[BGP] PolicyTable got modified route, deleting previous\n");
	rtmsg.route()->unref();
    }	
    
    if (!fmsg)
	return ADD_FILTERED;

    int res = next->add_route(*fmsg, this);

    if (fmsg != &rtmsg)
	delete fmsg;

    return res;
}

template <class A>
int
PolicyTable<A>::replace_route(const InternalMessage<A>& old_rtmsg,
			      const InternalMessage<A>& new_rtmsg,
			      BGPRouteTable<A>* caller)
{
    XLOG_ASSERT(caller == this->_parent);


    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    debug_msg("[BGP] PolicyTable %s replace_route: %s\nWith: %s\n",
	      filter::filter2str(_filter_type).c_str(),
	      old_rtmsg.str().c_str(),
	      new_rtmsg.str().c_str());

#if 0
    // the old one should really be old?
    switch (_filter_type) {
	case filter::IMPORT:
	   XLOG_ASSERT(!old_rtmsg.route()->policyfilter(0).is_empty());
	   break;

	case filter::EXPORT_SOURCEMATCH:
	   XLOG_ASSERT(!old_rtmsg.route()->policyfilter(1).is_empty());
	   break;

	case filter::EXPORT:
	   XLOG_ASSERT(!old_rtmsg.route()->policyfilter(2).is_empty());
	   break;
    }
    
    // the new route should really be new?
    switch (_filter_type) {
	case filter::IMPORT:
	   XLOG_ASSERT(new_rtmsg.route()->policyfilter(0).is_empty());
	   break;

	case filter::EXPORT_SOURCEMATCH:
	   XLOG_ASSERT(new_rtmsg.route()->policyfilter(1).is_empty());
	   break;

	case filter::EXPORT:
	   XLOG_ASSERT(new_rtmsg.route()->policyfilter(2).is_empty());
	   break;
    }
#endif

    const InternalMessage<A>* fold = do_filtering(old_rtmsg, false);
    if (old_rtmsg.changed() && fold != &old_rtmsg)
	old_rtmsg.route()->unref();
    const InternalMessage<A>* fnew = do_filtering(new_rtmsg, false);
    if (new_rtmsg.changed() && fnew != &new_rtmsg)
	new_rtmsg.route()->unref();

    // XXX: We can probably use the is_filtered flag...
    int res;

    // we didn't pass the old one, we don't want the new one
    if (fold == NULL && fnew == NULL) {
	return ADD_FILTERED;
    // we passed the old one, but we filtered the new one...
    } else if (fold != NULL && fnew == NULL) {
	next->delete_route(*fold, this);
	res = ADD_FILTERED;
    // we didn't pass the old one, we like the new one
    } else if (fold == NULL && fnew != NULL) {
	res = next->add_route(*fnew, this);
    // we passed tthe old one, we like the new one
    } else {
	res = next->replace_route(*fold, *fnew, this);
    }

    if (fold != &old_rtmsg)
	delete fold;
    if (fnew != &new_rtmsg)
	delete fnew;
   
    return res;
}

template <class A>
int
PolicyTable<A>::delete_route(const InternalMessage<A>& rtmsg,
			     BGPRouteTable<A>* caller)
{
    XLOG_ASSERT(caller == this->_parent);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    debug_msg("[BGP] PolicyTable %s delete_route: %s\n",
	      filter::filter2str(_filter_type).c_str(),
	      rtmsg.str().c_str());

#if 0
    // it must be an existing route...
    switch (_filter_type) {
	case filter::IMPORT:
	   XLOG_ASSERT(!rtmsg.route()->policyfilter(0).is_empty());
	   break;

	case filter::EXPORT_SOURCEMATCH:
	   XLOG_ASSERT(!rtmsg.route()->policyfilter(1).is_empty());
	   break;

	case filter::EXPORT:
	   XLOG_ASSERT(!rtmsg.route()->policyfilter(2).is_empty());
	   break;
    }
#endif

    const InternalMessage<A>* fmsg = do_filtering(rtmsg, false);
    if (rtmsg.changed() && fmsg != &rtmsg)
	rtmsg.route()->unref();

    if (fmsg == NULL)
	return 0;
    
    int res = next->delete_route(*fmsg, this);

    if (fmsg != &rtmsg)
	delete fmsg;

    return res;
}			     

template <class A>
int
PolicyTable<A>::route_dump(const InternalMessage<A>& rtmsg,
			   BGPRouteTable<A>* caller,
			   const PeerHandler* dump_peer)
{
    XLOG_ASSERT(caller == this->_parent);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    debug_msg("[BGP] PolicyTable %s route_dump: %s\n",
	      filter::filter2str(_filter_type).c_str(),
	      rtmsg.str().c_str());

#if 0
    // it must be an existing route...
    switch (_filter_type) {
	case filter::IMPORT:
	   XLOG_ASSERT(!rtmsg.route()->policyfilter(0).is_empty());
	   break;

	case filter::EXPORT_SOURCEMATCH:
	   XLOG_ASSERT(!rtmsg.route()->policyfilter(1).is_empty());
	   break;

	case filter::EXPORT:
	   XLOG_ASSERT(!rtmsg.route()->policyfilter(2).is_empty());
	   break;
    }
#endif

    const InternalMessage<A>* fmsg = do_filtering(rtmsg, false);
    if (rtmsg.changed() && &rtmsg != fmsg)
	rtmsg.route()->unref();
    
    if (fmsg == NULL)
	return ADD_FILTERED;

    int res = next->route_dump(*fmsg, this, dump_peer);

    if (fmsg != &rtmsg)
	delete fmsg;

    return res;
}			   

template <class A>
int
PolicyTable<A>::push(BGPRouteTable<A>* caller)
{
    XLOG_ASSERT(caller == this->_parent);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    return next->push(this);
}

template <class A>
const SubnetRoute<A>*
PolicyTable<A>::lookup_route(const IPNet<A> &net, 
			     uint32_t& genid) const
{
    BGPRouteTable<A>* parent = this->_parent;

    XLOG_ASSERT(parent);

    const SubnetRoute<A>* found = parent->lookup_route(net, genid);

    if (!found)
	return NULL;

    // We need the origin peer for matching neighbor!
    // XXX lame way of obtaining it:
    XLOG_ASSERT(_filter_type != filter::EXPORT);

    BGPRouteTable<A>* root = this->_parent;
    XLOG_ASSERT(root);

    while (root->parent() != NULL)
        root = root->parent();

    RibInTable<A>* ribin = dynamic_cast<RibInTable<A>* >(root);
    XLOG_ASSERT(ribin);

    InternalMessage<A> rtmsg(found, ribin->peer_handler(), genid);

    debug_msg("[BGP] PolicyTable %s lookup_route: %s\n",
	      filter::filter2str(_filter_type).c_str(),
	      rtmsg.str().c_str());

#if 0
    // it must be an existing route...
    switch (_filter_type) {
	case filter::IMPORT:
	   XLOG_ASSERT(!rtmsg.route()->policyfilter(0).is_empty());
	   break;

	case filter::EXPORT_SOURCEMATCH:
	   XLOG_ASSERT(!rtmsg.route()->policyfilter(1).is_empty());
	   break;

	case filter::EXPORT:
	   XLOG_ASSERT(!rtmsg.route()->policyfilter(2).is_empty());
	   break;
    }
#endif 

    const InternalMessage<A>* fmsg = do_filtering(rtmsg, false);
    
    
    if (!fmsg) {
	// consider static filters changing the message and we drop it.
	// that subnet route will leak!!!!
	// XXX unstable/lame/wrong way of detecting if route changed
	if (found->parent_route())
	    found->unref();
	return NULL;
    }
  
    // static filters didn't change it
    XLOG_ASSERT(found->parent_route() == NULL);
    // we didn't change it
    XLOG_ASSERT(fmsg == &rtmsg);
  
    return found;
}

template <class A>
string
PolicyTable<A>::str() const
{
    return "not implemented yet";
}

template <class A>
bool
PolicyTable<A>::get_next_message(BGPRouteTable<A>* next_table)
{
    BGPRouteTable<A>* parent = this->_parent;

    XLOG_ASSERT(parent);
    XLOG_ASSERT(this->_next_table == next_table);

    return parent->get_next_message(this);
}

template <class A>
void
PolicyTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use)
{
    BGPRouteTable<A>* parent = this->_parent;

    XLOG_ASSERT(parent);

    parent->route_used(rt, in_use);
}

template class PolicyTable<IPv4>;
template class PolicyTable<IPv6>;
