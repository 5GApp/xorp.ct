// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/process_watch_base.hh,v 1.5 2008/01/04 03:17:11 pavlin Exp $

#ifndef __POLICY_PROCESS_WATCH_BASE_HH__
#define __POLICY_PROCESS_WATCH_BASE_HH__

#include <string>

/**
 * @short Base class for a process watcher.
 *
 * The VarMap registers interest in known protocols. Finally, the filter manager
 * may be informed about which processes are alive.
 */
class ProcessWatchBase {
public:
    virtual ~ProcessWatchBase() {}

    /**
     * @param proto protocol to register interest in.
     */
    virtual void add_interest(const std::string& proto) = 0;
};

#endif // __POLICY_PROCESS_WATCH_BASE_HH__
