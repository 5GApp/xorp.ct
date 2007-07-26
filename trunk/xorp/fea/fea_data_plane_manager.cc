// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fea_data_plane_manager.cc,v 1.3 2007/07/18 01:30:22 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea_node.hh"
#include "fea_data_plane_manager.hh"


//
// FEA data plane manager base class implementation.
//

FeaDataPlaneManager::FeaDataPlaneManager(FeaNode& fea_node,
					 const string& manager_name)
    : _fea_node(fea_node),
      _ifconfig_get(NULL),
      _ifconfig_set(NULL),
      _ifconfig_observer(NULL),
      _fibconfig_forwarding(NULL),
      _fibconfig_entry_get(NULL),
      _fibconfig_entry_set(NULL),
      _fibconfig_entry_observer(NULL),
      _fibconfig_table_get(NULL),
      _fibconfig_table_set(NULL),
      _fibconfig_table_observer(NULL),
      _have_ipv4(false),
      _have_ipv6(false),
      _manager_name(manager_name),
      _is_loaded_plugins(false),
      _is_running_manager(false),
      _is_running_plugins(false)
{
}

FeaDataPlaneManager::~FeaDataPlaneManager()
{
    string error_msg;

    if (stop_manager(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop data plane manager %s: %s",
		   manager_name().c_str(), error_msg.c_str());
    }
}

int
FeaDataPlaneManager::start_manager(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running_manager)
	return (XORP_OK);

    if (load_plugins(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running_manager = true;

    return (XORP_OK);
}

int
FeaDataPlaneManager::stop_manager(string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (! _is_running_manager)
	return (XORP_OK);

    if (unload_plugins(error_msg2) != XORP_OK) {
	ret_value = XORP_ERROR;
	if (! error_msg.empty())
	    error_msg += " ";
	error_msg += error_msg2;
    }

    _is_running_manager = false;

    return (ret_value);
}

int
FeaDataPlaneManager::unload_plugins(string& error_msg)
{
    string error_msg2;

    UNUSED(error_msg);

    if (! _is_loaded_plugins)
	return (XORP_OK);

    if (stop_plugins(error_msg2) != XORP_OK) {
	XLOG_WARNING("Error during unloading the plugins for %s data plane "
		     "manager while stopping the plugins: %s. "
		     "Error ignored.",
		     manager_name().c_str(),
		     error_msg2.c_str());
    }

    //
    // Unload the plugins
    //
    if (_ifconfig_get != NULL) {
	delete _ifconfig_get;
	_ifconfig_get = NULL;
    }
    if (_ifconfig_set != NULL) {
	delete _ifconfig_set;
	_ifconfig_set = NULL;
    }
    if (_ifconfig_observer != NULL) {
	delete _ifconfig_observer;
	_ifconfig_observer = NULL;
    }
    if (_fibconfig_forwarding != NULL) {
	delete _fibconfig_forwarding;
	_fibconfig_forwarding = NULL;
    }
    if (_fibconfig_entry_get != NULL) {
	delete _fibconfig_entry_get;
	_fibconfig_entry_get = NULL;
    }
    if (_fibconfig_entry_set != NULL) {
	delete _fibconfig_entry_set;
	_fibconfig_entry_set = NULL;
    }
    if (_fibconfig_entry_observer != NULL) {
	delete _fibconfig_entry_observer;
	_fibconfig_entry_observer = NULL;
    }
    if (_fibconfig_table_get != NULL) {
	delete _fibconfig_table_get;
	_fibconfig_table_get = NULL;
    }
    if (_fibconfig_table_set != NULL) {
	delete _fibconfig_table_set;
	_fibconfig_table_set = NULL;
    }
    if (_fibconfig_table_observer != NULL) {
	delete _fibconfig_table_observer;
	_fibconfig_table_observer = NULL;
    }
    delete_pointers_list(_io_ip_list);
    delete_pointers_list(_io_link_list);

    _is_loaded_plugins = false;

    return (XORP_OK);
}

int
FeaDataPlaneManager::unregister_plugins(string& error_msg)
{
    UNUSED(error_msg);

    //
    // Unregister the plugins in the reverse order they were registered
    //
    io_link_manager().unregister_data_plane_manager(this);
    io_ip_manager().unregister_data_plane_manager(this);

    if (_fibconfig_table_observer != NULL)
	fibconfig().unregister_fibconfig_table_observer(_fibconfig_table_observer);
    if (_fibconfig_table_set != NULL)
	fibconfig().unregister_fibconfig_table_set(_fibconfig_table_set);
    if (_fibconfig_table_get != NULL)
	fibconfig().unregister_fibconfig_table_get(_fibconfig_table_get);
    if (_fibconfig_entry_observer != NULL)
	fibconfig().unregister_fibconfig_entry_observer(_fibconfig_entry_observer);
    if (_fibconfig_entry_set != NULL)
	fibconfig().unregister_fibconfig_entry_set(_fibconfig_entry_set);
    if (_fibconfig_entry_get != NULL)
	fibconfig().unregister_fibconfig_entry_get(_fibconfig_entry_get);
    if (_fibconfig_forwarding != NULL)
	fibconfig().unregister_fibconfig_forwarding(_fibconfig_forwarding);
    if (_ifconfig_observer != NULL)
	ifconfig().unregister_ifconfig_observer(_ifconfig_observer);
    if (_ifconfig_set != NULL)
	ifconfig().unregister_ifconfig_set(_ifconfig_set);
    if (_ifconfig_get != NULL)
	ifconfig().unregister_ifconfig_get(_ifconfig_get);

    return (XORP_OK);
}

int
FeaDataPlaneManager::start_plugins(string& error_msg)
{
    string dummy_error_msg;
    list<IoLink *>::iterator link_iter;
    list<IoIp *>::iterator ip_iter;

    if (_is_running_plugins)
	return (XORP_OK);

    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    if (register_plugins(error_msg) != XORP_OK) {
	error_msg = c_format("Cannot register plugins for data plane "
			     "manager %s: %s",
			     manager_name().c_str(),
			     error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // XXX: Start first the FibConfigForwarding so we can test whether
    // the data plane supports IPv4 and IPv6.
    //
    if (_fibconfig_forwarding != NULL) {
	if (_fibconfig_forwarding->start(error_msg) != XORP_OK)
	    goto error_label;
	_have_ipv4 = _fibconfig_forwarding->test_have_ipv4();
	_have_ipv6 = _fibconfig_forwarding->test_have_ipv6();
    }

    if (_ifconfig_get != NULL) {
	if (_ifconfig_get->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_ifconfig_set != NULL) {
	if (_ifconfig_set->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_ifconfig_observer != NULL) {
	if (_ifconfig_observer->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_entry_get != NULL) {
	if (_fibconfig_entry_get->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_entry_set != NULL) {
	if (_fibconfig_entry_set->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_entry_observer != NULL) {
	if (_fibconfig_entry_observer->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_table_get != NULL) {
	if (_fibconfig_table_get->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_table_set != NULL) {
	if (_fibconfig_table_set->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_table_observer != NULL) {
	if (_fibconfig_table_observer->start(error_msg) != XORP_OK)
	    goto error_label;
    }

    for (link_iter = _io_link_list.begin();
	 link_iter != _io_link_list.end();
	 ++link_iter) {
	IoLink* io_link = *link_iter;
	if (io_link->start(error_msg) != XORP_OK)
	    goto error_label;
    }

    for (ip_iter = _io_ip_list.begin();
	 ip_iter != _io_ip_list.end();
	 ++ip_iter) {
	IoIp* io_ip = *ip_iter;
	if (io_ip->start(error_msg) != XORP_OK)
	    goto error_label;
    }

    _is_running_plugins = true;

    return (XORP_OK);

 error_label:
    stop_all_plugins(dummy_error_msg);
    unregister_plugins(dummy_error_msg);
    return (XORP_ERROR);
}

int
FeaDataPlaneManager::stop_plugins(string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (! _is_running_plugins)
	return (XORP_OK);

    error_msg.erase();

    //
    // Stop the plugins
    //
    if (stop_all_plugins(error_msg2) != XORP_OK) {
	ret_value = XORP_ERROR;
	if (! error_msg.empty())
	    error_msg += " ";
	error_msg += error_msg2;
    }

    unregister_plugins(error_msg2);

    _have_ipv4 = false;
    _have_ipv6 = false;
    _is_running_plugins = false;

    return (ret_value);
}

int
FeaDataPlaneManager::register_all_plugins(bool is_exclusive, string& error_msg)
{
    string dummy_error_msg;

    if (_ifconfig_get != NULL) {
	if (ifconfig().register_ifconfig_get(_ifconfig_get, is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_ifconfig_set != NULL) {
	if (ifconfig().register_ifconfig_set(_ifconfig_set, is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigSet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_ifconfig_observer != NULL) {
	if (ifconfig().register_ifconfig_observer(_ifconfig_observer,
						  is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigObserver plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_forwarding != NULL) {
	if (fibconfig().register_fibconfig_forwarding(_fibconfig_forwarding,
						      is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigForwarding plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_entry_get != NULL) {
	if (fibconfig().register_fibconfig_entry_get(_fibconfig_entry_get,
						     is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigEntryGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_entry_set != NULL) {
	if (fibconfig().register_fibconfig_entry_set(_fibconfig_entry_set,
						     is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigEntrySet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_entry_observer != NULL) {
	if (fibconfig().register_fibconfig_entry_observer(_fibconfig_entry_observer,
							  is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigEntryObserver plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_table_get != NULL) {
	if (fibconfig().register_fibconfig_table_get(_fibconfig_table_get,
						     is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigTableGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_table_set != NULL) {
	if (fibconfig().register_fibconfig_table_set(_fibconfig_table_set,
						     is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigTableSet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_table_observer != NULL) {
	if (fibconfig().register_fibconfig_table_observer(_fibconfig_table_observer,
							  is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigTableObserver plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    //
    // XXX: The IoLink and IoIp plugins are registered on-demand
    // when a new client is registered.
    //

    return (XORP_OK);
}

int
FeaDataPlaneManager::stop_all_plugins(string& error_msg)
{
    list<IoIp *>::iterator ip_iter;
    list<IoLink *>::iterator link_iter;
    int ret_value = XORP_OK;
    string error_msg2;

    error_msg.erase();

    //
    // XXX: Stop the plugins in the reverse order they were started
    //
    for (ip_iter = _io_ip_list.begin();
	 ip_iter != _io_ip_list.end();
	 ++ip_iter) {
	IoIp* io_ip = *ip_iter;
	if (io_ip->stop(error_msg) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    for (link_iter = _io_link_list.begin();
	 link_iter != _io_link_list.end();
	 ++link_iter) {
	IoLink* io_link = *link_iter;
	if (io_link->stop(error_msg) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    if (_fibconfig_table_observer != NULL) {
	if (_fibconfig_table_observer->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_table_set != NULL) {
	if (_fibconfig_table_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_table_get != NULL) {
	if (_fibconfig_table_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_entry_observer != NULL) {
	if (_fibconfig_entry_observer->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_entry_set != NULL) {
	if (_fibconfig_entry_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_entry_get != NULL) {
	if (_fibconfig_entry_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_forwarding != NULL) {
	if (_fibconfig_forwarding->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_ifconfig_observer != NULL) {
	if (_ifconfig_observer->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_ifconfig_set != NULL) {
	if (_ifconfig_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_ifconfig_get != NULL) {
	if (_ifconfig_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

EventLoop&
FeaDataPlaneManager::eventloop()
{
    return (_fea_node.eventloop());
}

IfConfig&
FeaDataPlaneManager::ifconfig()
{
    return (_fea_node.ifconfig());
}

FibConfig&
FeaDataPlaneManager::fibconfig()
{
    return (_fea_node.fibconfig());
}

IoLinkManager&
FeaDataPlaneManager::io_link_manager()
{
    return (_fea_node.io_link_manager());
}

IoIpManager&
FeaDataPlaneManager::io_ip_manager()
{
    return (_fea_node.io_ip_manager());
}

void
FeaDataPlaneManager::deallocate_io_link(IoLink* io_link)
{
    list<IoLink *>::iterator iter;

    iter = find(_io_link_list.begin(), _io_link_list.end(), io_link);
    XLOG_ASSERT(iter != _io_link_list.end());
    _io_link_list.erase(iter);

    delete io_link;
}

void
FeaDataPlaneManager::deallocate_io_ip(IoIp* io_ip)
{
    list<IoIp *>::iterator iter;

    iter = find(_io_ip_list.begin(), _io_ip_list.end(), io_ip);
    XLOG_ASSERT(iter != _io_ip_list.end());
    _io_ip_list.erase(iter);

    delete io_ip;
}
