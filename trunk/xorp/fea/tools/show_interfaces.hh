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

// $XORP: xorp/fea/tools/show_interfaces.hh,v 1.8 2007/10/13 01:49:59 pavlin Exp $

#ifndef __FEA_TOOLS_SHOW_INTERFACES_HH__
#define __FEA_TOOLS_SHOW_INTERFACES_HH__


#include "libxorp/service.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"


class EventLoop;


class InterfaceMonitor : public IfMgrHintObserver,
			 public ServiceBase,
			 public ServiceChangeObserverBase {
public:
    /**
     * InterfaceMonitor constructor
     *
     * @param eventloop this process's EventLoop.
     * @param class_name the XRL class name of this target.
     * @param finder_hostname the finder's host name.
     * @param finder_port the finder's port.
     * @param fea_target the FEA target name.
     */
    InterfaceMonitor(EventLoop&		eventloop,
		     const string&	class_name,
		     const string&	finder_hostname,
		     uint16_t		finder_port,
		     const string&	fea_target);

    /**
     * InterfaceMonitor destructor
     */
    ~InterfaceMonitor();

    /**
     * Startup operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Shutdown operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Print information about network interfaces that was received
     * from the FEA.
     *
     * @param print_iface_name the name of the interface to print.
     * If it is the empty string, then print information about all
     * configured interfaces.
     */
    void print_interfaces(const string& print_iface_name) const;

protected:
    //
    // IfMgrHintObserver methods
    //
    void tree_complete();
    void updates_made();

    void incr_startup_requests_n();
    void decr_startup_requests_n();
    void incr_shutdown_requests_n();
    void decr_shutdown_requests_n();
    void update_status();

private:
    /**
     * A method invoked when the status of a service changes.
     * 
     * @param service the service whose status has changed.
     * @param old_status the old status.
     * @param new_status the new status.
     */
    void status_change(ServiceBase*  service,
		       ServiceStatus old_status,
		       ServiceStatus new_status);

    /**
     * Get a reference to the service base of the interface manager.
     * 
     * @return a reference to the service base of the interface manager.
     */
    const ServiceBase* ifmgr_mirror_service_base() const { return dynamic_cast<const ServiceBase*>(&_ifmgr); }

    /**
     * Get a reference to the interface manager tree.
     * 
     * @return a reference to the interface manager tree.
     */
    const IfMgrIfTree&	ifmgr_iftree() const { return _ifmgr.iftree(); }

    /**
     * Initiate startup of the interface manager.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int ifmgr_startup();

    /**
     * Initiate shutdown of the interface manager.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int ifmgr_shutdown();

    EventLoop&		_eventloop;

    IfMgrXrlMirror	_ifmgr;

    //
    // Status-related state
    //
    size_t		_startup_requests_n;
    size_t		_shutdown_requests_n;
};

#endif // __FEA_TOOLS_SHOW_INTERFACES_HH__
