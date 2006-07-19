// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/bgp/test_packet.hh,v 1.6 2005/08/18 15:58:08 bms Exp $

#ifndef __BGP_TEST_PACKET_HH__
#define __BGP_TEST_PACKET_HH__

#include "packet.hh"
#include "socket.hh"
#include "libxorp/debug.h"
#include "libxorp/exceptions.hh"
#include "libxorp/eventloop.hh"

class BGPTestPacket
{
public:
    BGPTestPacket();
    int run_tests();
protected:
private:
    KeepAlivePacket* create_keepalive();
    bool test_keepalive();
    UpdatePacket* create_update();
    bool test_update();
    UpdatePacket* create_update_ipv6();
    bool test_update_ipv6();
    OpenPacket* create_open();
    bool test_open();
    NotificationPacket* create_notification();
    bool test_notification();
    bool test_aspath();
};

#endif // __BGP_TEST_PACKET_HH__
