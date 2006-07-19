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

// $XORP: xorp/xrl/tests/test_xifs.hh,v 1.5 2005/03/25 02:55:15 pavlin Exp $

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_router.hh"

void try_common_xif_methods(EventLoop*	e, 
			    XrlRouter*	r, 
			    const char*	target_name);

void try_test_xif_methods(EventLoop*	e,
			  XrlRouter* 	r,
			  const char*	target_name);

