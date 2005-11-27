// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rtrmgr/config_operators.hh,v 1.3 2005/07/13 17:44:06 mjh Exp $

#ifndef __RTRMGR_CONFIG_OPERATORS_HH__
#define __RTRMGR_CONFIG_OPERATORS_HH__

#include <string>

#include "rtrmgr_error.hh"

//
// Configuration file operators.
// XXX: Comparators must be less than modifiers.
//
enum ConfigOperator {
    OP_NONE		= 0,
    OP_EQ		= 1,
    OP_NE		= 2,
    OP_LT		= 3,
    OP_LTE		= 4,
    OP_GT		= 5,
    OP_GTE		= 6,
    MAX_COMPARATOR	= OP_GTE,
    OP_ASSIGN		= 101,
    OP_ADD		= 102,
    OP_SUB		= 103,
    OP_DEL		= 104,
    MAX_MODIFIER	= OP_DEL
};

extern string operator_to_str(ConfigOperator op);
extern ConfigOperator lookup_operator(const string& s) throw (ParseError);

#endif // __RTRMGR_CONFIG_OPERATORS_HH__
