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

// $XORP: xorp/policy/test/filter_manager_fake.hh,v 1.2 2005/03/25 02:54:18 pavlin Exp $

#ifndef __POLICY_TEST_FILTER_MANAGER_FAKE_HH__
#define __POLICY_TEST_FILTER_MANAGER_FAKE_HH__

#include "policy/filter_manager_base.hh"

class FilterManagerFake : public FilterManagerBase {
public:
    void update_filter(const Code::Target& t);
    void flush_updates(uint32_t msec);
};

#endif // __POLICY_TEST_FILTER_MANAGER_FAKE_HH__
