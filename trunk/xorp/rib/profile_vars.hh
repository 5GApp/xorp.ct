// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/rib/profile_vars.hh,v 1.4 2007/02/16 22:47:07 pavlin Exp $

#ifndef __RIB_PROFILE_VARS_HH__
#define __RIB_PROFILE_VARS_HH__

/**
 * Profile variables
 * See: profile_vars.cc for definitions.
 */
const string profile_route_ribin = "route_ribin";
const string profile_route_rpc_in = "route_rpc_in";
const string profile_route_rpc_out = "route_rpc_out";

void initialize_profiling_variables(Profile& p);

#endif // __RIB_PROFILE_VARS_HH__
