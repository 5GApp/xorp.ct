// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fticonfig_entry_set_dummy.cc,v 1.13 2005/12/22 12:18:20 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_entry_set.hh"


//
// Set single-entry information into the unicast forwarding table.
//
// The mechanism to set the information is dummy (for testing purpose).
//


FtiConfigEntrySetDummy::FtiConfigEntrySetDummy(FtiConfig& ftic)
    : FtiConfigEntrySet(ftic)
{
#if 0	// XXX: by default Dummy is never registering by itself
    register_ftic_primary();
#endif
}

FtiConfigEntrySetDummy::~FtiConfigEntrySetDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the dummy mechanism to set "
		   "information about forwarding table from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FtiConfigEntrySetDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}
    
int
FtiConfigEntrySetDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}


bool
FtiConfigEntrySetDummy::add_entry4(const Fte4& fte)
{
    if (in_configuration() == false)
	return false;

    if (fte.is_connected_route())
	;	// XXX: accept directly-connected routes
    
    int rc = ftic().trie4().route_count();
    XLOG_ASSERT(rc >= 0);
    
    ftic().trie4().insert(fte.net(), fte);
    if (ftic().trie4().route_count() == rc) {
	XLOG_WARNING("add_entry4 is overriding old entry for %s (%d %d)",
		     fte.net().str().c_str(), rc, ftic().trie4().route_count());
    }
    
    return true;
}

bool
FtiConfigEntrySetDummy::delete_entry4(const Fte4& fte)
{
    if (in_configuration() == false)
	return false;

    if (fte.is_connected_route())
	;	// XXX: accept directly-connected routes
    
    Trie4::iterator ti = ftic().trie4().find(fte.net());
    if (ti == ftic().trie4().end())
	return false;
    ftic().trie4().erase(ti);
    
    return true;
}

bool
FtiConfigEntrySetDummy::add_entry6(const Fte6& fte)
{
    if (in_configuration() == false)
	return false;

    if (fte.is_connected_route())
	;	// XXX: accept directly-connected routes
    
    int rc = ftic().trie6().route_count();
    XLOG_ASSERT(rc >= 0);
    
    ftic().trie6().insert(fte.net(), fte);
    if (ftic().trie6().route_count() == rc) {
	XLOG_WARNING("add_entry6 is overriding old entry for %s (%d %d)",
		     fte.net().str().c_str(), rc, ftic().trie6().route_count());
    }
    
    return true;
}

bool
FtiConfigEntrySetDummy::delete_entry6(const Fte6& fte)
{
    if (in_configuration() == false)
	return false;

    if (fte.is_connected_route())
	;	// XXX: accept directly-connected routes
    
    Trie6::iterator ti = ftic().trie6().find(fte.net());
    if (ti == ftic().trie6().end())
	return false;
    ftic().trie6().erase(ti);
    
    return true;
}
