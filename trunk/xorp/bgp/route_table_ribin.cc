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

#ident "$XORP: xorp/bgp/route_table_ribin.cc,v 1.36 2005/03/18 08:15:04 mjh Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_ribin.hh"
#include "route_table_deletion.hh"
#include "rib_ipc_handler.hh"
#include "bgp.hh"

template<class A>
RibInTable<A>::RibInTable(string table_name,
			  Safi safi,
			  const PeerHandler *peer)
    : 	BGPRouteTable<A>("RibInTable-" + table_name, safi),
	_peer(peer)
{
    _route_table = new BgpTrie<A>;
    _peer_is_up = true;
    _genid = 1; /*zero is not a valid genid*/
    _table_version = 1;
    this->_parent = NULL;

    _nexthop_push_active = false;
}

template<class A>
RibInTable<A>::~RibInTable()
{
    delete _route_table;
}

template<class A>
void
RibInTable<A>::flush()
{
    debug_msg("%s\n", this->tablename().c_str());
    _route_table->delete_all_nodes();
}

template<class A>
void
RibInTable<A>::ribin_peering_went_down()
{
    _peer_is_up = false;

    // Stop pushing changed nexthops.
    stop_nexthop_push();

    /* When the peering goes down we unhook our entire RibIn routing
       table and give it to a new deletion table.  We plumb the
       deletion table in after the RibIn, and it can then delete the
       routes as a background task while filtering the routes we give
       it so that the later tables get the correct deletes at the
       correct time. This means that we don't have to concern
       ourselves with holding deleted and new routes if the peer comes
       back up before we've finished the background deletion process -
       credits to Atanu Ghosh for this neat idea */

    if (_route_table->route_count() > 0) {

	string tablename = "Deleted" + this->tablename();

	DeletionTable<A>* deletion_table =
	    new DeletionTable<A>(tablename, this->safi(), _route_table, _peer, 
				 _genid, this);

	_route_table = new BgpTrie<A>;

	deletion_table->set_next_table(this->_next_table);
	this->_next_table->set_parent(deletion_table);
	this->_next_table = deletion_table;

	this->_next_table->peering_went_down(_peer, _genid, this);
	deletion_table->initiate_background_deletion();
    } else {
	//nothing to delete - just notify everyone
	this->_next_table->peering_went_down(_peer, _genid, this);
	this->_next_table->push(this);
	this->_next_table->peering_down_complete(_peer, _genid, this);
    }
}

template<class A>
void
RibInTable<A>::ribin_peering_came_up()
{
    _peer_is_up = true;
    _genid++;

    // cope with wrapping genid without using zero which is reserved
    if (_genid == 0) {
	_genid = 1;
    }

    _table_version = 1;

    this->_next_table->peering_came_up(_peer, _genid, this);
}

template<class A>
int
RibInTable<A>::add_route(const InternalMessage<A> &rtmsg,
			 BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    const ChainedSubnetRoute<A> *new_route;
    const SubnetRoute<A> *existing_route;
    XLOG_ASSERT(caller == NULL);
    XLOG_ASSERT(rtmsg.origin_peer() == _peer);
    XLOG_ASSERT(_peer_is_up);
    XLOG_ASSERT(this->_next_table != NULL);

    uint32_t dummy;
    existing_route = lookup_route(rtmsg.net(), dummy);
    int response;
    if (existing_route != NULL) {
	XLOG_ASSERT(existing_route->net() == rtmsg.net());
#if 0
	if (rtmsg.route()->attributes() == existing_route->attributes()) {
	    // this route is the same as before.
	    // no need to do anything.
	    return ADD_UNUSED;
	}
#endif
	// Preserve the route.  Taking a reference will prevent the
	// route being deleted when it's erased from the Trie.
	// Deletion will occur when the reference goes out of scope.
	SubnetRouteConstRef<A> route_reference(existing_route);
	deletion_nexthop_check(existing_route);

	// delete from the Trie
	_route_table->erase(rtmsg.net());
	_table_version++;

	InternalMessage<A> old_rt_msg(existing_route, _peer, _genid);

	// Store it locally.  The BgpTrie will copy it into a ChainedSubnetRoute
	typename BgpTrie<A>::iterator iter =
	    _route_table->insert(rtmsg.net(), *(rtmsg.route()));
	new_route = &(iter.payload());

	// propagate downstream
	InternalMessage<A> new_rt_msg(new_route, _peer, _genid);
	if (rtmsg.push()) new_rt_msg.set_push();

	response = this->_next_table->replace_route(old_rt_msg, new_rt_msg,
					      (BGPRouteTable<A>*)this);
    } else {
	// Store it locally.  The BgpTrie will copy it into a ChainedSubnetRoute
	typename BgpTrie<A>::iterator iter =
	    _route_table->insert(rtmsg.net(), *(rtmsg.route()));
	new_route = &(iter.payload());

	// progogate downstream
	InternalMessage<A> new_rt_msg(new_route, _peer, _genid);
	if (rtmsg.push()) new_rt_msg.set_push();
	response = this->_next_table->add_route(new_rt_msg, (BGPRouteTable<A>*)this);
    }

    switch (response) {
    case ADD_UNUSED:
	new_route->set_in_use(false);
	new_route->set_filtered(false);
	break;
    case ADD_FILTERED:
	new_route->set_in_use(false);
	new_route->set_filtered(true);
	break;
    case ADD_USED:
    case ADD_FAILURE:
	// the default if we don't know for sure that a route is unused
	// should be that it is used.
	new_route->set_in_use(true);
	new_route->set_filtered(false);
	break;
    }

    return response;
}

template<class A>
int
RibInTable<A>::delete_route(const InternalMessage<A> &rtmsg,
			    BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    XLOG_ASSERT(caller == NULL);
    XLOG_ASSERT(rtmsg.origin_peer() == _peer);
    XLOG_ASSERT(_peer_is_up);

    const SubnetRoute<A> *existing_route;
    uint32_t dummy;
    existing_route = lookup_route(rtmsg.net(), dummy);

    if (existing_route != NULL) {
	// Preserve the route.  Taking a reference will prevent the
	// route being deleted when it's erased from the Trie.
	// Deletion will occur when the reference goes out of scope.
	SubnetRouteConstRef<A> route_reference(existing_route);
	deletion_nexthop_check(existing_route);

	// remove from the Trie
	_route_table->erase(rtmsg.net());
	_table_version++;

	// propogate downstream
	InternalMessage<A> old_rt_msg(existing_route, _peer, _genid);
	if (rtmsg.push()) old_rt_msg.set_push();
	if (this->_next_table != NULL)
	    this->_next_table->delete_route(old_rt_msg, (BGPRouteTable<A>*)this);
    } else {
	// we received a delete, but didn't have anything to delete.
	// It's debatable whether we should silently ignore this, or
	// drop the peering.  If we don't hold input-filtered routes in
	// the RIB-In, then this would be commonplace, so we'd have to
	// silently ignore it.  Currently (Sept 2002) we do still hold
	// filtered routes in the RIB-In, so this should be an error.
	// But we'll just ignore this error, and log a warning.
	string s = "Attempt to delete route for net " + rtmsg.net().str()
	    + " that wasn't in RIB-In\n";
	XLOG_WARNING(s.c_str());
	return -1;
    }
    return 0;
}

template<class A>
int
RibInTable<A>::push(BGPRouteTable<A> *caller)
{
    debug_msg("RibInTable<A>::push\n");
    XLOG_ASSERT(caller == NULL);
    XLOG_ASSERT(_peer_is_up);
    XLOG_ASSERT(this->_next_table != NULL);

    return this->_next_table->push((BGPRouteTable<A>*)this);
}

template<class A>
const SubnetRoute<A>*
RibInTable<A>::lookup_route(const IPNet<A> &net, uint32_t& genid) const
{
    if (_peer_is_up == false)
	return NULL;

    typename BgpTrie<A>::iterator iter = _route_table->lookup_node(net);
    if (iter != _route_table->end()) {
	// assert(iter.payload().net() == net);
	genid = _genid;
	return &(iter.payload());
    } else
	return NULL;
}

template<class A>
void
RibInTable<A>::route_used(const SubnetRoute<A>* used_route, bool in_use)
{
    // we look this up rather than modify used_route itself because
    // it's possible that used_route originates the other side of a
    // cache, and modifying it might not modify the version we have
    // here in the RibIn
    if (_peer_is_up == false)
	return;
    const SubnetRoute<A> *rt;
    uint32_t dummy;
    rt = lookup_route(used_route->net(), dummy);
    XLOG_ASSERT(rt != NULL);
    rt->set_in_use(in_use);
}

template<class A>
string
RibInTable<A>::str() const
{
    string s = "RibInTable<A>" + this->tablename();
    return s;
}

template<class A>
bool
RibInTable<A>::dump_next_route(DumpIterator<A>& dump_iter)
{
    typename BgpTrie<A>::iterator route_iterator;
    debug_msg("dump iter: %s\n", dump_iter.str().c_str());
   
    if (dump_iter.route_iterator_is_valid()) {
	debug_msg("route_iterator is valid\n");
 	route_iterator = dump_iter.route_iterator();
	// Make sure the iterator is valid. If it is pointing at a
	// deleted node this comparison will move it forward.
	if (route_iterator == _route_table->end()) {
	    return false;
	}
	
	//we need to move on to the next node, except if the iterator
	//was pointing at a deleted node, because then it will have
	//just been moved to the next node to dump, so we need to dump
	//the node that the iterator is currently pointing at.
	if (dump_iter.iterator_got_moved(route_iterator.key()) == false)
	    route_iterator++;
    } else {
	debug_msg("route_iterator is not valid\n");
	route_iterator = _route_table->begin();
    }

    if (route_iterator == _route_table->end()) {
	return false;
    }

    const ChainedSubnetRoute<A>* chained_rt;
    for ( ; route_iterator != _route_table->end(); route_iterator++) {
	chained_rt = &(route_iterator.payload());
	debug_msg("chained_rt: %s\n", chained_rt->str().c_str());

	// propagate downstream
	// only dump routes that actually won
	// XXX: or if its a policy route dump

	if (chained_rt->is_winner() || dump_iter.peer_to_dump_to() == NULL) {
	    InternalMessage<A> rt_msg(chained_rt, _peer, _genid);
	    rt_msg.set_push();
	   
	    int res = this->_next_table->route_dump(rt_msg, (BGPRouteTable<A>*)this,
				    dump_iter.peer_to_dump_to());

	    if(res == ADD_FILTERED) 
		chained_rt->set_filtered(true);
	    else
		chained_rt->set_filtered(false);
	    break;
	}
    }

    if (route_iterator == _route_table->end())
	return false;

    // Store the new iterator value as its valid.
    dump_iter.set_route_iterator(route_iterator);

    return true;
}

template<class A>
void
RibInTable<A>::igp_nexthop_changed(const A& bgp_nexthop)
{
    debug_msg("igp_nexthop_changed for bgp_nexthop %s on table %s\n",
	   bgp_nexthop.str().c_str(), this->tablename().c_str());

    typename set <A>::const_iterator i;
    i = _changed_nexthops.find(bgp_nexthop);
    if (i != _changed_nexthops.end()) {
	debug_msg("nexthop already queued\n");
	// this nexthop is already queued to be pushed again
	return;
    }

    if (_nexthop_push_active) {
	debug_msg("push active, adding to queue\n");
	_changed_nexthops.insert(bgp_nexthop);
    } else {
	debug_msg("push was inactive, activating\n");
	PathAttributeList<A> dummy_pa_list;
	NextHopAttribute<A> nh_att(bgp_nexthop);
	dummy_pa_list.add_path_attribute(nh_att);
	dummy_pa_list.rehash();
	typename BgpTrie<A>::PathmapType::const_iterator pmi;
#ifdef NOTDEF
	pmi = _route_table->pathmap().begin();
	while (pmi != _route_table->pathmap().end()) {
	    debug_msg("Route: %s\n", pmi->second->str().c_str());
	    pmi++;
	}
#endif

	pmi = _route_table->pathmap().lower_bound(&dummy_pa_list);
	if (pmi == _route_table->pathmap().end()) {
	    // no route in this trie has this Nexthop
	    debug_msg("no matching routes - do nothing\n");
	    return;
	}
	if (pmi->second->nexthop() != bgp_nexthop) {
	    debug_msg("no matching routes (2)- do nothing\n");
	    return;
	}
	_current_changed_nexthop = bgp_nexthop;
	_nexthop_push_active = true;
	_current_chain = pmi;
	const SubnetRoute<A>* next_route_to_push = _current_chain->second;
	UNUSED(next_route_to_push);
	debug_msg("Found route with nexthop %s:\n%s\n",
		  bgp_nexthop.str().c_str(),
		  next_route_to_push->str().c_str());

	// _next_table->push((BGPRouteTable<A>*)this);
	_push_timer = eventloop().
	    new_oneoff_after_ms(0 /*call back immediately, but after
				    network events or expired timers */,
				callback(this,
					 &RibInTable<A>::push_next_changed_nexthop));
    }
}

template<class A>
void
RibInTable<A>::push_next_changed_nexthop()
{
    XLOG_ASSERT(_peer_is_up);

    const ChainedSubnetRoute<A>* chained_rt, *first_rt;
    first_rt = chained_rt = _current_chain->second;
    while (1) {
	// Replace the route with itself.  This will cause filters to
	// be re-applied, and decision to re-evaluate the route.
	InternalMessage<A> old_rt_msg(chained_rt, _peer, _genid);
	InternalMessage<A> new_rt_msg(chained_rt, _peer, _genid);

	//we used to send this as a replace route, but replacing a
	//route with itself isn't safe in terms of preserving the
	//flags.
	this->_next_table->delete_route(old_rt_msg, (BGPRouteTable<A>*)this);
	this->_next_table->add_route(new_rt_msg, (BGPRouteTable<A>*)this);

	if (chained_rt->next() == first_rt) {
	    debug_msg("end of chain\n");
	    break;
	} else {
	    debug_msg("chain continues\n");
	}
	chained_rt = chained_rt->next();
    }
    debug_msg("****RibIn Sending push***\n");
    this->_next_table->push((BGPRouteTable<A>*)this);

    next_chain();

    if (_nexthop_push_active == false)
	return;

    _push_timer = eventloop().
	new_oneoff_after_ms(0 /*call back immediately, but after
				network events or expired timers */,
			    callback(this,
				     &RibInTable<A>::push_next_changed_nexthop));
}


template<class A>
void
RibInTable<A>::deletion_nexthop_check(const SubnetRoute<A>* route)
{
    // checks to make sure that the deletion doesn't make
    // _next_route_to_push invalid
    if (!_nexthop_push_active)
	return;
    const ChainedSubnetRoute<A>* next_route_to_push = _current_chain->second;
    if (*route == *next_route_to_push) {
	if (next_route_to_push == next_route_to_push->next()) {
	    // this is the last route in the chain, so we need to bump
	    // the iterator before we delete it.
	    next_chain();
	}
    }
}

template<class A>
void
RibInTable<A>::next_chain()
{
    _current_chain++;
    if (_current_chain != _route_table->pathmap().end()
	&& _current_chain->first->nexthop() == _current_changed_nexthop) {
	// there's another chain with the same nexthop
	return;
    }

    while (1) {
	// that's it for this nexthop - try the next
	if (_changed_nexthops.empty()) {
	    // no more nexthops to push
	    _nexthop_push_active = false;
	    return;
	}

	typename set <A>::iterator i = _changed_nexthops.begin();
	_current_changed_nexthop = *i;
	_changed_nexthops.erase(i);

	PathAttributeList<A> dummy_pa_list;
	NextHopAttribute<A> nh_att(_current_changed_nexthop);
	dummy_pa_list.add_path_attribute(nh_att);
	dummy_pa_list.rehash();
	typename BgpTrie<A>::PathmapType::const_iterator pmi;

	pmi = _route_table->pathmap().lower_bound(&dummy_pa_list);
	if (pmi == _route_table->pathmap().end()) {
	    // no route in this trie has this Nexthop, try the next nexthop
	    continue;
	}
	if (pmi->second->nexthop() != _current_changed_nexthop) {
	    // no route in this trie has this Nexthop, try the next nexthop
	    continue;
	}

	_current_chain = pmi;
	return;
    }
}

template<class A>
void
RibInTable<A>::stop_nexthop_push()
{
    // When a peering goes down tear out all the nexthop change state

    _changed_nexthops.clear();
    _nexthop_push_active = false;
    _current_changed_nexthop = A::ZERO();
    _current_chain = _route_table->pathmap().end();
    _push_timer.unschedule();
}

template<class A>
EventLoop& 
RibInTable<A>::eventloop() const 
{
    return _peer->eventloop();
}


template class RibInTable<IPv4>;
template class RibInTable<IPv6>;
