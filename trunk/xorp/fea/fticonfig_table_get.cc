// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/fea/fticonfig_table_get.cc,v 1.7 2005/03/05 01:41:22 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fticonfig.hh"
#include "fticonfig_table_get.hh"


//
// Get the whole table information from the unicast forwarding table.
//


FtiConfigTableGet::FtiConfigTableGet(FtiConfig& ftic)
    : _s4(-1),
      _s6(-1),
      _is_running(false),
      _ftic(ftic),
      _is_primary(true)
{
    
}

FtiConfigTableGet::~FtiConfigTableGet()
{
    if (_s4 >= 0) {
	close(_s4);
	_s4 = -1;
    }
    if (_s6 >= 0) {
	close(_s6);
	_s6 = -1;
    }
}

void
FtiConfigTableGet::register_ftic_primary()
{
    _ftic.register_ftic_table_get_primary(this);
}

void
FtiConfigTableGet::register_ftic_secondary()
{
    _ftic.register_ftic_table_get_secondary(this);
}

int
FtiConfigTableGet::sock(int family)
{
    switch (family) {
    case AF_INET:
	return _s4;
#ifdef HAVE_IPV6
    case AF_INET6:
	return _s6;
#endif // HAVE_IPV6
    default:
	XLOG_FATAL("Unknown address family %d", family);
    }
    return (-1);
}
