// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/test_sample_config.hh,v 1.1 2004/12/23 22:00:01 mjh Exp $

#ifndef __RTRMGR_TEST_MODULE_MANAGER_HH__
#define __RTRMGR_TEST_MODULE_MANAGER_HH__

#include "generic_module_manager.hh"

class Rtrmgr {
public:
    Rtrmgr();
    int run();
    void module_status_changed(const string& module_name,
			       GenericModule::ModuleStatus status);
};

#endif // __RTRMGR_TEST_MODULE_MANAGER_HH__
