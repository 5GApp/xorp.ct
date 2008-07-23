// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/libxipc/xuid.hh,v 1.8 2008/01/04 03:16:29 pavlin Exp $

#ifndef __LIBXIPC_XUID_HH__
#define __LIBXIPC_XUID_HH__

#include <string>

class XUID {
public:
    class InvalidString {};
    XUID(const string&) throw (class InvalidString);

    // an XUID can be explicitly constructed
    XUID() { initialize(); }

    // Or a block of memory can be cast as an XUID and initialized.
    void initialize();

    bool operator==(const XUID&) const;
    bool operator<(const XUID&) const;

    string str() const;
private:
    uint32_t _data[4];		// Internal representation is network ordered
};

#endif // __LIBXIPC_XUID_HH__
