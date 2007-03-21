// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/common/elem_null.hh,v 1.5 2006/03/16 00:05:16 pavlin Exp $

#ifndef __POLICY_COMMON_ELEM_NULL_HH__
#define __POLICY_COMMON_ELEM_NULL_HH__

#include "element_base.hh"
#include <string>

/**
 * @short An element representing nothing. Null.
 *
 * This is used by VarRW when an element being read is not available. For
 * example, if the route is IPv4, but the IPv6 representation is being asked
 * for.
 *
 * The dispatcher also treats Null arguments as a special case by returning a
 * Null.
 */
class ElemNull : public Element {
public:
    static const char* id;
    static Hash _hash;
            
    ElemNull() {}
    ElemNull(const char* /* c_str */) {}
                
    string str() const { return "null"; }

    const char* type() const { return id; }
    Hash hash() const { return _hash; } 
    void set_hash(const Hash& x) { _hash = x; }
                
};

#endif // __POLICY_COMMON_ELEM_NULL_HH__
