/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2005 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 */

/*
 * $XORP: xorp/libxorp/ether_compat.h,v 1.8 2004/06/22 23:41:42 pavlin Exp $
 */

/* Ethernet manipulation compatibility functions */

#ifndef __LIBXORP_ETHER_COMPAT_H__
#define __LIBXORP_ETHER_COMPAT_H__

#ifndef __XORP_CONFIG_H__
#error "config.h must be included before ether_compat.h"
#endif /* __XORP_CONFIG_H__ */

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#elif defined(HAVE_NET_IF_ETHER_H)
#include <net/if.h>
#include <net/if_ether.h>
#elif defined(HAVE_SYS_ETHERNET_H)
#include <sys/types.h>
#include <sys/ethernet.h>
#elif defined(HAVE_NETINET_IF_ETHER_H)
#include <net/if.h>
#include <netinet/if_ether.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef NEED_ETHER_ATON
struct ether_addr* ether_aton(const char *a);
#endif /* NEED_ETHER_ATON */

#ifdef NEED_ETHER_NTOA
char* ether_ntoa(const struct ether_addr* ea);
#endif /* NEED_ETHER_NTOA */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LIBXORP_ETHER_COMPAT_H__ */
