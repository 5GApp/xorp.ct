// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/data_plane/managers/fea_data_plane_manager_dummy.cc,v 1.10 2008/04/26 00:59:48 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/data_plane/ifconfig/ifconfig_property_dummy.hh"
#include "fea/data_plane/ifconfig/ifconfig_get_dummy.hh"
#include "fea/data_plane/ifconfig/ifconfig_set_dummy.hh"
#include "fea/data_plane/ifconfig/ifconfig_observer_dummy.hh"
#include "fea/data_plane/ifconfig/ifconfig_vlan_get_dummy.hh"
#include "fea/data_plane/ifconfig/ifconfig_vlan_set_dummy.hh"
#include "fea/data_plane/firewall/firewall_get_dummy.hh"
#include "fea/data_plane/firewall/firewall_set_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_forwarding_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_get_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_set_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_observer_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_get_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_set_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_observer_dummy.hh"
#include "fea/data_plane/io/io_link_dummy.hh"
#include "fea/data_plane/io/io_ip_dummy.hh"
#include "fea/data_plane/io/io_tcpudp_dummy.hh"

#include "fea_data_plane_manager_dummy.hh"


//
// FEA data plane manager class for Dummy FEA.
//

#if 0
extern "C" FeaDataPlaneManager* create(FeaNode& fea_node)
{
    return (new FeaDataPlaneManagerDummy(fea_node));
}

extern "C" void destroy(FeaDataPlaneManager* fea_data_plane_manager)
{
    delete fea_data_plane_manager;
}
#endif // 0


FeaDataPlaneManagerDummy::FeaDataPlaneManagerDummy(FeaNode& fea_node)
    : FeaDataPlaneManager(fea_node, "Dummy")
{
}

FeaDataPlaneManagerDummy::~FeaDataPlaneManagerDummy()
{
}

int
FeaDataPlaneManagerDummy::load_plugins(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_loaded_plugins)
	return (XORP_OK);

    XLOG_ASSERT(_ifconfig_property == NULL);
    XLOG_ASSERT(_ifconfig_get == NULL);
    XLOG_ASSERT(_ifconfig_set == NULL);
    XLOG_ASSERT(_ifconfig_observer == NULL);
    XLOG_ASSERT(_ifconfig_vlan_get == NULL);
    XLOG_ASSERT(_ifconfig_vlan_set == NULL);
    XLOG_ASSERT(_firewall_get == NULL);
    XLOG_ASSERT(_firewall_set == NULL);
    XLOG_ASSERT(_fibconfig_forwarding == NULL);
    XLOG_ASSERT(_fibconfig_entry_get == NULL);
    XLOG_ASSERT(_fibconfig_entry_set == NULL);
    XLOG_ASSERT(_fibconfig_entry_observer == NULL);
    XLOG_ASSERT(_fibconfig_table_get == NULL);
    XLOG_ASSERT(_fibconfig_table_set == NULL);
    XLOG_ASSERT(_fibconfig_table_observer == NULL);

    //
    // Load the plugins
    //
    _ifconfig_property = new IfConfigPropertyDummy(*this);
    _ifconfig_get = new IfConfigGetDummy(*this);
    _ifconfig_set = new IfConfigSetDummy(*this);
    _ifconfig_observer = new IfConfigObserverDummy(*this);
    _ifconfig_vlan_get = new IfConfigVlanGetDummy(*this);
    _ifconfig_vlan_set = new IfConfigVlanSetDummy(*this);
    _firewall_get = new FirewallGetDummy(*this);
    _firewall_set = new FirewallSetDummy(*this);
    _fibconfig_forwarding = new FibConfigForwardingDummy(*this);
    _fibconfig_entry_get = new FibConfigEntryGetDummy(*this);
    _fibconfig_entry_set = new FibConfigEntrySetDummy(*this);
    _fibconfig_entry_observer = new FibConfigEntryObserverDummy(*this);
    _fibconfig_table_get = new FibConfigTableGetDummy(*this);
    _fibconfig_table_set = new FibConfigTableSetDummy(*this);
    _fibconfig_table_observer = new FibConfigTableObserverDummy(*this);

    _is_loaded_plugins = true;

    return (XORP_OK);
}

int
FeaDataPlaneManagerDummy::register_plugins(string& error_msg)
{
    return (FeaDataPlaneManager::register_all_plugins(true, error_msg));
}

IoLink*
FeaDataPlaneManagerDummy::allocate_io_link(const IfTree& iftree,
					   const string& if_name,
					   const string& vif_name,
					   uint16_t ether_type,
					   const string& filter_program)
{
    IoLink* io_link = NULL;

    io_link = new IoLinkDummy(*this, iftree, if_name, vif_name, ether_type,
			      filter_program);
    _io_link_list.push_back(io_link);

    return (io_link);
}

IoIp*
FeaDataPlaneManagerDummy::allocate_io_ip(const IfTree& iftree, int family,
					 uint8_t ip_protocol)
{
    IoIp* io_ip = NULL;

    io_ip = new IoIpDummy(*this, iftree, family, ip_protocol);
    _io_ip_list.push_back(io_ip);

    return (io_ip);
}

IoTcpUdp*
FeaDataPlaneManagerDummy::allocate_io_tcpudp(const IfTree& iftree, int family,
					     bool is_tcp)
{
    IoTcpUdp* io_tcpudp = NULL;

    io_tcpudp = new IoTcpUdpDummy(*this, iftree, family, is_tcp);
    _io_tcpudp_list.push_back(io_tcpudp);

    return (io_tcpudp);
}
