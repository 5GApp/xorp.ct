// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP$

#ifndef __OSPF_EXCEPTIONS_HH__
#define __OSPF_EXCEPTIONS_HH__

/**
 * Bad Packet exception.
 */
class BadPacket : public XorpReasonedException
{
public:
    BadPacket(const char* file, size_t line, const string init_why = "")
 	: XorpReasonedException("BadPacket", file, line, init_why)
    {}
};

/**
 * Bad Peer exception.
 */
class BadPeer : public XorpReasonedException
{
public:
    BadPeer(const char* file, size_t line, const string init_why = "")
 	: XorpReasonedException("BadPeer", file, line, init_why)
    {}
};

#endif // __OSPF_EXCEPTIONS_HH__
