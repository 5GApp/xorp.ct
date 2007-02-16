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

// $XORP: xorp/policy/test/process_watch_fake.hh,v 1.3 2006/03/16 00:05:24 pavlin Exp $

#ifndef __POLICY_TEST_PROCESS_WATCH_FAKE_HH__
#define __POLICY_TEST_PROCESS_WATCH_FAKE_HH__

#include "policy/process_watch_base.hh"

class ProcessWatchFake : public ProcessWatchBase {
public:
    void add_interest(const string& proto);
};

#endif // __POLICY_TEST_PROCESS_WATCH_FAKE_HH__
