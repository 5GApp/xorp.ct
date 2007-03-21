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

#ident "$XORP: xorp/fea/fticonfig_table_set_netlink.cc,v 1.11 2006/10/17 22:26:08 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_table_set.hh"


//
// Set whole-table information into the unicast forwarding table.
//
// The mechanism to set the information is netlink(7) sockets.
//


FtiConfigTableSetNetlink::FtiConfigTableSetNetlink(FtiConfig& ftic)
    : FtiConfigTableSet(ftic)
{
#ifdef HAVE_NETLINK_SOCKETS
    register_ftic_primary();
#endif
}

FtiConfigTableSetNetlink::~FtiConfigTableSetNetlink()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink(7) sockets mechanism to set "
		   "whole forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FtiConfigTableSetNetlink::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    // Cleanup any leftover entries from previously run XORP instance
    if (! ftic().unicast_forwarding_entries_retain_on_startup4())
	delete_all_entries4();
    if (! ftic().unicast_forwarding_entries_retain_on_startup6())
	delete_all_entries6();

    //
    // XXX: This mechanism relies on the FtiConfigEntrySet mechanism
    // to set the forwarding table, hence there is nothing else to do.
    //

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigTableSetNetlink::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    // Delete the XORP entries
    if (! ftic().unicast_forwarding_entries_retain_on_shutdown4())
	delete_all_entries4();
    if (! ftic().unicast_forwarding_entries_retain_on_shutdown6())
	delete_all_entries6();

    //
    // XXX: This mechanism relies on the FtiConfigEntrySet mechanism
    // to set the forwarding table, hence there is nothing else to do.
    //

    _is_running = false;

    return (XORP_OK);
}

bool
FtiConfigTableSetNetlink::set_table4(const list<Fte4>& fte_list)
{
    list<Fte4>::const_iterator iter;

    // Add the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte4& fte = *iter;
	ftic().add_entry4(fte);
    }
    
    return true;
}

bool
FtiConfigTableSetNetlink::delete_all_entries4()
{
    list<Fte4> fte_list;
    list<Fte4>::const_iterator iter;
    
    // Get the list of all entries
    ftic().get_table4(fte_list);
    
    // Delete the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte4& fte = *iter;
	if (fte.xorp_route())
	    ftic().delete_entry4(fte);
    }
    
    return true;
}

bool
FtiConfigTableSetNetlink::set_table6(const list<Fte6>& fte_list)
{
    list<Fte6>::const_iterator iter;
    
    // Add the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte6& fte = *iter;
	ftic().add_entry6(fte);
    }
    
    return true;
}
    
bool
FtiConfigTableSetNetlink::delete_all_entries6()
{
    list<Fte6> fte_list;
    list<Fte6>::const_iterator iter;
    
    // Get the list of all entries
    ftic().get_table6(fte_list);
    
    // Delete the entries one-by-one
    for (iter = fte_list.begin(); iter != fte_list.end(); ++iter) {
	const Fte6& fte = *iter;
	if (fte.xorp_route())
	    ftic().delete_entry6(fte);
    }
    
    return true;
}
