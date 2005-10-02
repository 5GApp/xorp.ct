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

// $XORP: xorp/policy/common/element_base.hh,v 1.2 2005/03/25 02:54:16 pavlin Exp $

#ifndef __POLICY_COMMON_ELEMENT_BASE_HH__
#define __POLICY_COMMON_ELEMENT_BASE_HH__

#include <string>

/**
 * @short Basic object type used by policy engine.
 *
 * This element hierarchy is similar to XrlAtom's but exclusive to policy
 * components.
 */
class Element {
public:
    typedef unsigned char Hash;

    virtual ~Element() {}

    /**
     * Every element must be representable by a string. This is a requirement
     * to enable the policy manager to send elements to the backend filters via
     * XRL calls for example.
     *
     * @return string representation of the element.
     */
    virtual std::string str() const = 0;

    /**
     * @return string representation of element type.
     */
    virtual const char* type() const = 0;

    virtual Hash hash() const = 0;
    virtual void set_hash(const Hash&) = 0;
};

#endif // __POLICY_COMMON_ELEMENT_BASE_HH__
