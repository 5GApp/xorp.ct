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

// $XORP: xorp/fea/ifconfig_media.hh,v 1.1 2005/10/16 07:10:35 pavlin Exp $

#ifndef __FEA_IFCONFIG_MEDIA_HH__
#define __FEA_IFCONFIG_MEDIA_HH__

extern int
ifconfig_media_get_link_status(const string& if_name, bool& no_carrier,
			       string& error_msg);

#endif // __FEA_IFCONFIG_MEDIA_HH__
