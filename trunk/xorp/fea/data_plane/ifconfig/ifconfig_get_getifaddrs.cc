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

#ident "$XORP: xorp/fea/forwarding_plane/ifconfig/ifconfig_get_getifaddrs.cc,v 1.1 2007/04/25 07:31:55 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_get.hh"


//
// Get information about network interfaces from the underlying system.
//
// The mechanism to obtain the information is getifaddrs(3).
//


IfConfigGetGetifaddrs::IfConfigGetGetifaddrs(IfConfig& ifconfig)
    : IfConfigGet(ifconfig)
{
#ifdef HAVE_GETIFADDRS
    register_ifconfig_primary();
#endif
}

IfConfigGetGetifaddrs::~IfConfigGetGetifaddrs()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the getifaddrs(3) mechanism to get "
		   "information about network interfaces from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigGetGetifaddrs::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigGetGetifaddrs::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

bool
IfConfigGetGetifaddrs::pull_config(IfTree& iftree)
{
    return read_config(iftree);
}

#ifndef HAVE_GETIFADDRS
bool
IfConfigGetGetifaddrs::read_config(IfTree& )
{
    return false;
}

#else // HAVE_GETIFADDRS
bool
IfConfigGetGetifaddrs::read_config(IfTree& it)
{
    struct ifaddrs *ifap;
    
    if (getifaddrs(&ifap) != 0) {
	XLOG_ERROR("getifaddrs() failed: %s", strerror(errno));
	return false;
    }

    parse_buffer_ifaddrs(it, ifap);

    freeifaddrs(ifap);
    
    return true;
}

#endif // HAVE_GETIFADDRS
