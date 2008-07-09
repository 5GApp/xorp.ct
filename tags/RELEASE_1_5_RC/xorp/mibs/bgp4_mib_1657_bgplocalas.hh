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

// $XORP: xorp/mibs/bgp4_mib_1657_bgplocalas.hh,v 1.6 2007/02/16 22:46:33 pavlin Exp $


#ifndef __MIBS_BGP4_MIB_1657_BGPLOCALAS_HH__
#define __MIBS_BGP4_MIB_1657_BGPLOCALAS_HH__

void init_bgp4_mib_1657_bgplocalas(void);

/* callback functions that will be called from snmpd, thus the
   C linkage */
extern "C" {

Netsnmp_Node_Handler get_bgpLocalAs;

}

#endif // __MIBS_BGP4_MIB_1657_BGPLOCALAS_HH__
