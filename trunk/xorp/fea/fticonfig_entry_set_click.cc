// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/fea/fticonfig_entry_set_click.cc,v 1.22 2005/03/16 01:36:58 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_entry_set.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to obtain the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//


FtiConfigEntrySetClick::FtiConfigEntrySetClick(FtiConfig& ftic)
    : FtiConfigEntrySet(ftic),
      ClickSocket(ftic.eventloop()),
      _cs_reader(*(ClickSocket *)this),
      _reinstall_all_entries_time_slice(100000, 20),	// 100ms, test every 20th iter
      _start_reinstalling_fte_table4(false),
      _is_reinstalling_fte_table4(false),
      _start_reinstalling_fte_table6(false),
      _is_reinstalling_fte_table6(false),
      _reinstalling_ipv4net(IPv4::ZERO(), 0),
      _reinstalling_ipv6net(IPv6::ZERO(), 0)
{
}

FtiConfigEntrySetClick::~FtiConfigEntrySetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to set "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FtiConfigEntrySetClick::start(string& error_msg)
{
    if (! ClickSocket::is_enabled())
	return (XORP_OK);

    if (_is_running)
	return (XORP_OK);

    if (ClickSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    // XXX: add myself as an observer to the NexthopPortMapper
    ftic().nexthop_port_mapper().add_observer(this);

    _is_running = true;

    //
    // XXX: we should register ourselves after we are running so the
    // registration process itself can trigger some startup operations
    // (if any).
    //
    if (ClickSocket::is_duplicate_routes_to_kernel_enabled())
	register_ftic_secondary();
    else
	register_ftic_primary();

    return (XORP_OK);
}

int
FtiConfigEntrySetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    // XXX: delete myself as an observer to the NexthopPortMapper
    ftic().nexthop_port_mapper().delete_observer(this);

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

bool
FtiConfigEntrySetClick::add_entry4(const Fte4& fte)
{
    bool ret_value;

    if (fte.is_connected_route())
	;	// XXX: accept directly-connected routes

    FteX ftex(fte);
    
    ret_value = add_entry(ftex);

    // Add the entry to the local copy of the forwarding table
    if (ret_value) {
	map<IPv4Net, Fte4>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table4.find(fte.net());
	if (iter != _fte_table4.end())
	    _fte_table4.erase(iter);

	_fte_table4.insert(make_pair(fte.net(), fte));
    }

    return (ret_value);
}

bool
FtiConfigEntrySetClick::delete_entry4(const Fte4& fte)
{
    bool ret_value;

    if (fte.is_connected_route())
	;	// XXX: accept directly-connected routes

    FteX ftex(fte);
    
    ret_value = delete_entry(ftex);

    // Delete the entry from the local copy of the forwarding table
    if (ret_value) {
	map<IPv4Net, Fte4>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table4.find(fte.net());
	if (iter != _fte_table4.end())
	    _fte_table4.erase(iter);
    }

    return (ret_value);
}

bool
FtiConfigEntrySetClick::add_entry6(const Fte6& fte)
{
    bool ret_value;

    if (fte.is_connected_route())
	;	// XXX: accept directly-connected routes

    FteX ftex(fte);
    
    ret_value = add_entry(ftex);

    // Add the entry to the local copy of the forwarding table
    if (ret_value) {
	map<IPv6Net, Fte6>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table6.find(fte.net());
	if (iter != _fte_table6.end())
	    _fte_table6.erase(iter);

	_fte_table6.insert(make_pair(fte.net(), fte));
    }

    return (ret_value);
}

bool
FtiConfigEntrySetClick::delete_entry6(const Fte6& fte)
{
    bool ret_value;

    if (fte.is_connected_route())
	;	// XXX: accept directly-connected routes

    FteX ftex(fte);
    
    ret_value = delete_entry(ftex);

    // Delete the entry from the local copy of the forwarding table
    if (ret_value) {
	map<IPv6Net, Fte6>::iterator iter;

	// Erase the entry with the same key (if exists)
	iter = _fte_table6.find(fte.net());
	if (iter != _fte_table6.end())
	    _fte_table6.erase(iter);
    }

    return (ret_value);
}

bool
FtiConfigEntrySetClick::add_entry(const FteX& fte)
{
    int port = -1;
    string element;
    string handler = "add";
    string error_msg;

    debug_msg("add_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! ftic().have_ipv4())
		return (false);
	    element = "_xorp_rt4";
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! ftic().have_ipv6())
		return (false);
	    element = "_xorp_rt6";
	    break;
	}
	XLOG_UNREACHABLE();
	break;
    } while (false);

    //
    // Get the outgoing port number
    //
    do {
	NexthopPortMapper& m = ftic().nexthop_port_mapper();
	const IPvX& nexthop = fte.nexthop();
	bool lookup_nexthop_interface_first = true;

	if (fte.is_connected_route()
	    && (IPvXNet(nexthop, nexthop.addr_bitlen()) == fte.net())) {
	    //
	    // XXX: if a directly-connected route, then check whether the
	    // next-hop address is same as the destination address. This
	    // could be the case of either:
	    // (a) the other side of a p2p link
	    // OR
	    // (b) a connected route for a /32 or /128 configured interface
	    // We are interested in discovering case (b) only, but anyway
	    // in both cases we then lookup the MextPortMapper by trying
	    // first the nexthop address.
	    //
	    // Note that we don't want to map all "connected" routes
	    // by the next-hop address, because typically the next-hop
	    // address will be local IP address, and we don't want to
	    // mis-route the traffic for all directly connected destinations
	    // to the local delivery port of the Click lookup element.
	    //
	    lookup_nexthop_interface_first = false;
	}
	if (lookup_nexthop_interface_first) {
	    port = m.lookup_nexthop_interface(fte.ifname(), fte.vifname());
	    if (port >= 0)
		break;
	}
	if (nexthop.is_ipv4()) {
	    port = m.lookup_nexthop_ipv4(nexthop.get_ipv4());
	    if (port >= 0)
		break;
	}
	if (nexthop.is_ipv6()) {
	    port = m.lookup_nexthop_ipv6(nexthop.get_ipv6());
	    if (port >= 0)
		break;
	}
	if (! lookup_nexthop_interface_first) {
	    port = m.lookup_nexthop_interface(fte.ifname(), fte.vifname());
	    if (port >= 0)
		break;
	}
	break;
    } while (false);

    if (port < 0) {
	XLOG_ERROR("Cannot find outgoing port number for the Click forwarding "
		   "table element to add entry %s", fte.str().c_str());
	return (false);
    }

    //
    // Write the configuration
    //
    string config;
    if (fte.is_connected_route()) {
	config = c_format("%s %d\n",
			  fte.net().str().c_str(),
			  port);
    } else {
	config = c_format("%s %s %d\n",
			  fte.net().str().c_str(),
			  fte.nexthop().str().c_str(),
			  port);
    }

    //
    // XXX: here we always write the same config to both kernel and
    // user-level Click.
    //
    bool has_kernel_config = true;
    bool has_user_config = true;
    if (ClickSocket::write_config(element, handler,
				  has_kernel_config, config,
				  has_user_config, config,
				  error_msg)
	!= XORP_OK) {
	XLOG_ERROR("%s", error_msg.c_str());
	return (false);
    }

    return (true);
}

bool
FtiConfigEntrySetClick::delete_entry(const FteX& fte)
{
    int port = -1;
    string element;
    string handler = "remove";
    string error_msg;

    debug_msg("delete_entry "
	      "(network = %s nexthop = %s)",
	      fte.net().str().c_str(), fte.nexthop().str().c_str());

    // Check that the family is supported
    do {
	if (fte.nexthop().is_ipv4()) {
	    if (! ftic().have_ipv4())
		return (false);
	    element = "_xorp_rt4";
	    break;
	}
	if (fte.nexthop().is_ipv6()) {
	    if (! ftic().have_ipv6())
		return (false);
	    element = "_xorp_rt6";
	    break;
	}
	XLOG_UNREACHABLE();
	break;
    } while (false);

    //
    // Get the outgoing port number
    //
    do {
	NexthopPortMapper& m = ftic().nexthop_port_mapper();
	port = m.lookup_nexthop_interface(fte.ifname(), fte.vifname());
	if (port >= 0)
	    break;
	if (fte.nexthop().is_ipv4()) {
	    port = m.lookup_nexthop_ipv4(fte.nexthop().get_ipv4());
	    if (port >= 0)
		break;
	}
	if (fte.nexthop().is_ipv6()) {
	    port = m.lookup_nexthop_ipv6(fte.nexthop().get_ipv6());
	    if (port >= 0)
		break;
	}
	break;
    } while (false);

#if 0	// TODO: XXX: currently, we don't have the next-hop info, hence
	// we allow to delete an entry without the port number.
    if (port < 0) {
	XLOG_ERROR("Cannot find outgoing port number for the Click forwarding "
		   "table element to delete entry %s", fte.str().c_str());
	return (false);
    }
#endif // 0

    //
    // Write the configuration
    //
    string config;
    if (port < 0) {
	// XXX: remove all routes for a given prefix
	//
	// TODO: XXX: currently, we don't have the next-hop info,
	// hence we delete all routes for a given prefix. After all,
	// in case of XORP+Click we always install no more than
	// a single route per prefix.
	//
	config = c_format("%s\n",
			  fte.net().str().c_str());
    } else {
	// XXX: remove a specific route
	if (fte.is_connected_route()) {
	    config = c_format("%s %d\n",
			      fte.net().str().c_str(),
			      port);
	} else {
	    config = c_format("%s %s %d\n",
			      fte.net().str().c_str(),
			      fte.nexthop().str().c_str(),
			      port);
	}
    }

    //
    // XXX: here we always write the same config to both kernel and
    // user-level Click.
    //
    bool has_kernel_config = true;
    bool has_user_config = true;
    if (ClickSocket::write_config(element, handler,
				  has_kernel_config, config,
				  has_user_config, config,
				  error_msg)
	!= XORP_OK) {
	XLOG_ERROR("%s", error_msg.c_str());
	return (false);
    }

    return (true);
}

void
FtiConfigEntrySetClick::nexthop_port_mapper_event(bool is_mapping_changed)
{
    //
    // XXX: always reinstall all entries, because they were lost
    // when the new Click config was written.
    //
    start_task_reinstall_all_entries();
    UNUSED(is_mapping_changed);
}

void
FtiConfigEntrySetClick::start_task_reinstall_all_entries()
{
    // Initialize the reinstalling from the beginning
    _start_reinstalling_fte_table4 = true;
    _is_reinstalling_fte_table4 = false;
    _start_reinstalling_fte_table6 = true;
    _is_reinstalling_fte_table6 = false;

    run_task_reinstall_all_entries();
}

void
FtiConfigEntrySetClick::run_task_reinstall_all_entries()
{
    _reinstall_all_entries_time_slice.reset();

    //
    // Reinstall all IPv4 entries. If the time slice expires, then schedule
    // a timer to continue later.
    //
    if (_start_reinstalling_fte_table4 || _is_reinstalling_fte_table4) {
	if (reinstall_all_entries4() == true) {
	    _reinstall_all_entries_timer = ftic().eventloop().new_oneoff_after(
		TimeVal(0, 1),
		callback(this, &FtiConfigEntrySetClick::run_task_reinstall_all_entries));
	    return;
	}
    }

    //
    // Reinstall all IPv6 entries. If the time slice expires, then schedule
    // a timer to continue later.
    //
    if (_start_reinstalling_fte_table6 || _is_reinstalling_fte_table6) {
	if (reinstall_all_entries6() == true) {
	    _reinstall_all_entries_timer = ftic().eventloop().new_oneoff_after(
		TimeVal(0, 1),
		callback(this, &FtiConfigEntrySetClick::run_task_reinstall_all_entries));
	    return;
	}
    }

    return;
}

bool
FtiConfigEntrySetClick::reinstall_all_entries4()
{
    map<IPv4Net, Fte4>::const_iterator iter4, iter4_begin, iter4_end;

    // Set the begin and end iterators
    if (_start_reinstalling_fte_table4) {
	iter4_begin = _fte_table4.begin();
    } else if (_is_reinstalling_fte_table4) {
	iter4_begin = _fte_table4.lower_bound(_reinstalling_ipv4net);
    } else {
	return (false);		// XXX: nothing to do
    }
    iter4_end = _fte_table4.end();

    _start_reinstalling_fte_table4 = false;
    _is_reinstalling_fte_table4 = true;

    for (iter4 = iter4_begin; iter4 != iter4_end; ) {
	const FteX& ftex = FteX(iter4->second);
	++iter4;
	add_entry(ftex);
	if (_reinstall_all_entries_time_slice.is_expired()) {
	    // Time slice has expired. Save state if necessary and return.
	    if (iter4 != iter4_end) {
		const Fte4& fte4_next = iter4->second;
		_reinstalling_ipv4net = fte4_next.net();
	    } else {
		_is_reinstalling_fte_table4 = false;
	    }
	    return (true);
	}
    }
    _is_reinstalling_fte_table4 = false;

    return (false);
}

bool
FtiConfigEntrySetClick::reinstall_all_entries6()
{
    map<IPv6Net, Fte6>::const_iterator iter6, iter6_begin, iter6_end;

    // Set the begin and end iterators
    if (_start_reinstalling_fte_table6) {
	iter6_begin = _fte_table6.begin();
    } else if (_is_reinstalling_fte_table6) {
	iter6_begin = _fte_table6.lower_bound(_reinstalling_ipv6net);
    } else {
	return (false);		// XXX: nothing to do
    }
    iter6_end = _fte_table6.end();

    _start_reinstalling_fte_table6 = false;
    _is_reinstalling_fte_table6 = true;

    for (iter6 = iter6_begin; iter6 != iter6_end; ) {
	const FteX& ftex = FteX(iter6->second);
	++iter6;
	add_entry(ftex);
	if (_reinstall_all_entries_time_slice.is_expired()) {
	    // Time slice has expired. Save state if necessary and return.
	    if (iter6 != iter6_end) {
		const Fte6& fte6_next = iter6->second;
		_reinstalling_ipv6net = fte6_next.net();
	    } else {
		_is_reinstalling_fte_table6 = false;
	    }
	    return (true);
	}
    }
    _is_reinstalling_fte_table6 = false;

    return (false);
}
