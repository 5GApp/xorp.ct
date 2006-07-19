// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/common/elem_filter.hh,v 1.1 2005/10/02 22:21:54 abittau Exp $

#ifndef __POLICY_COMMON_ELEM_FILTER_HH__
#define __POLICY_COMMON_ELEM_FILTER_HH__

#include "policy/backend/policy_filter.hh"
#include "element_base.hh"
#include "libxorp/ref_ptr.hh"
#include <string>

/**
 * @short a filter element.  Used when versioning.
 */
class ElemFilter : public Element {
public:
    static Hash _hash;
    
    ElemFilter(const RefPf& pf) : _pf(pf) {}
    string str() const { return "policy filter"; }
    const RefPf& val() const { return _pf; }

    void set_hash(const Hash& x) { _hash = x; }
    Hash hash() const { return _hash; }
    const char* type() const { return "filter"; }

private:
    RefPf _pf;
};

#endif // __POLICY_COMMON_ELEM_FILTER_HH__
