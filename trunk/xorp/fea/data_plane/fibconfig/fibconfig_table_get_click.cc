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

#ident "$XORP: xorp/fea/forwarding_plane/fibconfig/fibconfig_table_get_click.cc,v 1.2 2007/04/26 22:29:56 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvxnet.hh"

#include "fibconfig.hh"
#include "fibconfig_table_get.hh"


//
// Get the whole table information from the unicast forwarding table.
//
// The mechanism to obtain the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//


FibConfigTableGetClick::FibConfigTableGetClick(FibConfig& fibconfig)
    : FibConfigTableGet(fibconfig),
      ClickSocket(fibconfig.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
}

FibConfigTableGetClick::~FibConfigTableGetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to get "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FibConfigTableGetClick::start(string& error_msg)
{
    if (! ClickSocket::is_enabled())
	return (XORP_OK);

    if (_is_running)
	return (XORP_OK);

    if (ClickSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    //
    // XXX: we should register ourselves after we are running so the
    // registration process itself can trigger some startup operations
    // (if any).
    //
    // XXX: we register ourselves as the primary "get" method, because
    // Click should be the ultimate place to read the route info from.
    // The kernel itself may contain some left-over stuff.
    //
    register_fibconfig_primary();

    return (XORP_OK);
}

int
FibConfigTableGetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

bool
FibConfigTableGetClick::get_table4(list<Fte4>& fte_list)
{
    const map<IPv4Net, Fte4>& fte_table4 = fibconfig().fibconfig_entry_set_click().fte_table4();
    map<IPv4Net, Fte4>::const_iterator iter;

    for (iter = fte_table4.begin(); iter != fte_table4.end(); ++iter) {
	fte_list.push_back(iter->second);
    }
    
    return true;
}

bool
FibConfigTableGetClick::get_table6(list<Fte6>& fte_list)
{
#ifndef HAVE_IPV6
    UNUSED(fte_list);
    
    return false;
#else
    const map<IPv6Net, Fte6>& fte_table6 = fibconfig().fibconfig_entry_set_click().fte_table6();
    map<IPv6Net, Fte6>::const_iterator iter;

    for (iter = fte_table6.begin(); iter != fte_table6.end(); ++iter) {
	fte_list.push_back(iter->second);
    }
    
    return true;
#endif // HAVE_IPV6
}
