// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/libxipc/xrl_pf_kill.hh,v 1.7 2007/05/23 12:12:40 pavlin Exp $

#ifndef __LIBXIPC_XRL_PF_KILL_HH__
#define __LIBXIPC_XRL_PF_KILL_HH__

#include "xrl_pf.hh"

class XUID;

class XrlPFKillSender : public XrlPFSender {
public:
    XrlPFKillSender(EventLoop& e, const char* pid_str) throw (XrlPFConstructorError);
    virtual ~XrlPFKillSender();

    bool send(const Xrl& 			x,
	      bool 				direct_call,
	      const XrlPFSender::SendCallback& 	cb);

    bool sends_pending() const;

    bool alive() const;

    const char* protocol() const;

    static const char* protocol_name()		{ return _protocol; }

protected:
    int _pid;
    
    static const char _protocol[];
};

#endif // __LIBXIPC_XRL_PF_KILL_HH__
