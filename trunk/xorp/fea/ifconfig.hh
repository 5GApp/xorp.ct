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

// $XORP: xorp/fea/ifconfig.hh,v 1.45 2005/10/12 08:51:54 pavlin Exp $

#ifndef __FEA_IFCONFIG_HH__
#define __FEA_IFCONFIG_HH__

#include "ifconfig_get.hh"
#include "ifconfig_set.hh"
#include "ifconfig_observer.hh"
#include "iftree.hh"

class EventLoop;
class IfConfigGet;
class IfConfigSet;
class IfConfigObserver;
class IfConfigErrorReporterBase;
class IfConfigUpdateReporterBase;
class NexthopPortMapper;


/**
 * Base class for pushing and pulling interface configurations down to
 * the particular platform.
 */
class IfConfig {
public:
    /**
     * Constructor.
     *
     * @param eventloop the event loop.
     * @param ur update reporter that receives updates through when
     *           configurations are pushed down and when triggered
     *		 spontaneously on the underlying platform.
     * @param er error reporter that errors are propagated through when
     *           configurations are pushed down.
     * @param nexthop_port_mapper the next-hop port mapper.
     */
    IfConfig(EventLoop& eventloop, IfConfigUpdateReporterBase& ur,
	     IfConfigErrorReporterBase& er,
	     NexthopPortMapper& nexthop_port_mapper);

    /**
     * Virtual destructor (in case this class is used as base class).
     */
    virtual ~IfConfig();

    /**
     * Get a reference to the @ref EventLoop instance.
     *
     * @return a reference to the @ref EventLoop instance.
     */
    EventLoop& eventloop() { return _eventloop; }

    /**
     * Get a reference to the @ref NexthopPortMapper instance.
     *
     * @return a reference to the @ref NexthopPortMapper instance.
     */
    NexthopPortMapper& nexthop_port_mapper() { return _nexthop_port_mapper; }

    /**
     * Get error reporter associated with IfConfig.
     */
    inline IfConfigErrorReporterBase&	er() { return _er; }

    IfTree& live_config() { return (_live_config); }
    void    set_live_config(const IfTree& it) { _live_config = it; }

    const IfTree& pulled_config()	{ return (_pulled_config); }
    IfTree& pushed_config()		{ return (_pushed_config); }

    IfTree* local_config() { return (_local_config); }
    void    hook_local_config(IfTree* v) { _local_config = v; }

    /**
     * Test whether the original configuration should be restored on shutdown.
     *
     * @return true of the original configuration should be restored on
     * shutdown, otherwise false.
     */
    bool restore_original_config_on_shutdown() const {
	return (_restore_original_config_on_shutdown);
    }

    /**
     * Set the flag whether the original configuration should be restored
     * on shutdown.
     *
     * @param v if true the original configuration should be restored on
     * shutdown.
     */
    void set_restore_original_config_on_shutdown(bool v) {
	_restore_original_config_on_shutdown = v;
    }

    int register_ifc_get_primary(IfConfigGet *ifc_get);
    int register_ifc_set_primary(IfConfigSet *ifc_set);
    int register_ifc_observer_primary(IfConfigObserver *ifc_observer);
    int register_ifc_get_secondary(IfConfigGet *ifc_get);
    int register_ifc_set_secondary(IfConfigSet *ifc_set);
    int register_ifc_observer_secondary(IfConfigObserver *ifc_observer);

    IfConfigGet&	ifc_get_primary() { return *_ifc_get_primary; }
    IfConfigSet&	ifc_set_primary() { return *_ifc_set_primary; }
    IfConfigObserver&	ifc_observer_primary() { return *_ifc_observer_primary; }

    IfConfigGet&	ifc_get_ioctl() { return _ifc_get_ioctl; }
    IfConfigSetClick&	ifc_set_click() { return _ifc_set_click; }

    /**
     * Setup the unit to behave as dummy (for testing purpose).
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_dummy();

    /**
     * Test if running in dummy mode.
     * 
     * @return true if running in dummy mode, otherwise false.
     */
    bool is_dummy() const { return _is_dummy; }

    /**
     * Start operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start(string& error_msg);

    /**
     * Stop operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop(string& error_msg);

    /**
     * Return true if the underlying system supports IPv4.
     * 
     * @return true if the underlying system supports IPv4, otherwise false.
     */
    bool have_ipv4() const { return _have_ipv4; }

    /**
     * Return true if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    bool have_ipv6() const { return _have_ipv6; }

    /**
     * Test if the underlying system supports IPv4.
     * 
     * @return true if the underlying system supports IPv4, otherwise false.
     */
    bool test_have_ipv4() const;

    /**
     * Test if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    bool test_have_ipv6() const;

    /**
     * Enable/disable Click support.
     *
     * @param enable if true, then enable Click support, otherwise disable it.
     */
    void enable_click(bool enable);

    /**
     * Start Click support.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start_click(string& error_msg);

    /**
     * Stop Click support.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop_click(string& error_msg);

    /**
     * Specify the external program to generate the kernel-level Click
     * configuration.
     *
     * @param v the name of the external program to generate the kernel-level
     * Click configuration.
     */
    void set_kernel_click_config_generator_file(const string& v);

    /**
     * Enable/disable kernel-level Click support.
     *
     * @param enable if true, then enable the kernel-level Click support,
     * otherwise disable it.
     */
    void enable_kernel_click(bool enable);

    /**
     * Enable/disable installing kernel-level Click on startup.
     *
     * @param enable if true, then install kernel-level Click on startup.
     */
    void enable_kernel_click_install_on_startup(bool enable);

    /**
     * Specify the list of kernel Click modules to load on startup if
     * installing kernel-level Click on startup is enabled.
     *
     * @param modules the list of kernel Click modules to load.
     */
    void set_kernel_click_modules(const list<string>& modules);

    /**
     * Specify the kernel-level Click mount directory.
     *
     * @param directory the kernel-level Click mount directory.
     */
    void set_kernel_click_mount_directory(const string& directory);

    /**
     * Enable/disable user-level Click support.
     *
     * @param enable if true, then enable the user-level Click support,
     * otherwise disable it.
     */
    void enable_user_click(bool enable);

    /**
     * Specify the user-level Click command file.
     *
     * @param v the name of the user-level Click command file.
     */
    void set_user_click_command_file(const string& v);

    /**
     * Specify the extra arguments to the user-level Click command.
     *
     * @param v the extra arguments to the user-level Click command.
     */
    void set_user_click_command_extra_arguments(const string& v);

    /**
     * Specify whether to execute on startup the user-level Click command.
     *
     * @param v if true, then execute the user-level Click command on startup.
     */
    void set_user_click_command_execute_on_startup(bool v);

    /**
     * Specify the address to use for control access to the user-level
     * Click.
     *
     * @param v the address to use for control access to the user-level Click.
     */
    void set_user_click_control_address(const IPv4& v);

    /**
     * Specify the socket port to use for control access to the user-level
     * Click.
     *
     * @param v the socket port to use for control access to the user-level
     * Click.
     */
    void set_user_click_control_socket_port(uint32_t v);

    /**
     * Specify the configuration file to be used by user-level Click on
     * startup.
     *
     * @param v the name of the configuration file to be used by user-level
     * Click on startup.
     */
    void set_user_click_startup_config_file(const string& v);

    /**
     * Specify the external program to generate the user-level Click
     * configuration.
     *
     * @param v the name of the external program to generate the user-level
     * Click configuration.
     */
    void set_user_click_config_generator_file(const string& v);

    /**
     * Push IfTree structure down to platform.  Errors are reported
     * via the constructor supplied ErrorReporter instance.
     *
     * Note that on return some of the interface tree configuration state
     * may be modified.
     *
     * @param config the configuration to be pushed down.
     * @return true on success, otherwise false.
     */
    bool push_config(IfTree& config);

    /**
     * Pull up current config from platform.
     *
     * @return the platform IfTree.
     */
    const IfTree& pull_config();

    void flush_config() { _live_config = IfTree(); }

    IfTreeInterface *get_if(IfTree& it, const string& ifname);
    IfTreeVif *get_vif(IfTree& it, const string& ifname,
		       const string& vifname);

    /**
     * Get error message associated with push operation.
     */
    const string& push_error() const;

    /**
     * Check IfTreeInterface and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface& fi,
			 bool is_system_interfaces_reportee);

    /**
     * Check IfTreeVif and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface& fi, const IfTreeVif& fv,
			 bool is_system_interfaces_reportee);

    /**
     * Check IfTreeAddr4 and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface&	fi,
			 const IfTreeVif&	fv,
			 const IfTreeAddr4&     fa,
			 bool  is_system_interfaces_reportee);

    /**
     * Check IfTreeAddr6 and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface&	fi,
			 const IfTreeVif&	fv,
			 const IfTreeAddr6&     fa,
			 bool  is_system_interfaces_reportee);

    /**
     * Report that updates were completed to IfConfigUpdateReporter.
     */
    void   report_updates_completed(bool is_system_interfaces_reportee);

    /**
     * Check every item within IfTree and report updates to
     * IfConfigUpdateReporter.
     */
    void   report_updates(const IfTree& it, bool is_system_interfaces_reportee);

    void	map_ifindex(uint32_t if_index, const string& ifname);
    void	unmap_ifindex(uint32_t if_index);
    const char*	find_ifname(uint32_t if_index) const;
    uint32_t	find_ifindex(const string& ifname) const;

    const char*	get_insert_ifname(uint32_t if_index);
    uint32_t	get_insert_ifindex(const string& ifname);

private:

    EventLoop&			_eventloop;
    IfConfigUpdateReporterBase&	_ur;
    IfConfigErrorReporterBase&	_er;
    NexthopPortMapper&		_nexthop_port_mapper;

    //
    // A cache of associative array of interface names to interface index.
    // Needed because the RTM_IFANNOUNCE upcall is called after the interface
    // name is deleted from the kernel.
    //
    typedef map<uint32_t, string> IfIndex2NameMap;
    IfIndex2NameMap	_ifnames;

    IfTree		_live_config;	// The IfTree with live config
    IfTree		_pulled_config;	// The IfTree when we pull the config
    IfTree		_pushed_config;	// The IfTree when we push the config
    IfTree		_original_config; // The IfTree on startup
    bool		_restore_original_config_on_shutdown; // If true, then
				//  restore the original config on shutdown
    IfTree*		_local_config;	// The IfTree with the local config

    IfConfigGet*		_ifc_get_primary;
    IfConfigSet*		_ifc_set_primary;
    IfConfigObserver*		_ifc_observer_primary;
    list<IfConfigGet*>		_ifc_gets_secondary;
    list<IfConfigSet*>		_ifc_sets_secondary;
    list<IfConfigObserver*>	_ifc_observers_secondary;

    //
    // The primary mechanisms to get interface-related information
    // from the underlying system.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    IfConfigGetDummy		_ifc_get_dummy;
    IfConfigGetIoctl		_ifc_get_ioctl;
    IfConfigGetSysctl		_ifc_get_sysctl;
    IfConfigGetGetifaddrs	_ifc_get_getifaddrs;
    IfConfigGetProcLinux	_ifc_get_proc_linux;
    IfConfigGetNetlink		_ifc_get_netlink;
    IfConfigGetIPHelper		_ifc_get_iphelper;

    //
    // The secondary mechanisms to get interface-related information
    // from the underlying system.
    //
    // XXX: Ordering is not important.
    //
    IfConfigGetClick		_ifc_get_click;

    //
    // The primary mechanisms to set interface-related information
    // within the underlying system.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    IfConfigSetDummy		_ifc_set_dummy;
    IfConfigSetIoctl		_ifc_set_ioctl;
    IfConfigSetNetlink		_ifc_set_netlink;
    IfConfigSetIPHelper		_ifc_set_iphelper;

    //
    // The secondary mechanisms to get interface-related information
    // from the underlying system.
    //
    // XXX: Ordering is not important.
    //
    IfConfigSetClick		_ifc_set_click;

    //
    // The primary mechanisms to observe whether the interface-related
    // information within the underlying system has changed.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    IfConfigObserverDummy	_ifc_observer_dummy;
    IfConfigObserverRtsock	_ifc_observer_rtsock;
    IfConfigObserverNetlink	_ifc_observer_netlink;
    IfConfigObserverIPHelper	_ifc_observer_iphelper;

    //
    // Misc other state
    //
    bool	_have_ipv4;
    bool	_have_ipv6;
    bool	_is_dummy;
    bool	_is_running;
};

/**
 * @short Base class for propagating update information on from IfConfig.
 *
 * When the platform @ref IfConfig updates interfaces it can report
 * updates to an IfConfigUpdateReporter.  The @ref IfConfig instance
 * takes a pointer to the IfConfigUpdateReporter object it should use.
 */
class IfConfigUpdateReporterBase {
public:
    enum Update { CREATED, DELETED, CHANGED };

    virtual ~IfConfigUpdateReporterBase() {}

    virtual void interface_update(const string& ifname,
				  const Update& u,
				  bool  is_system_interfaces_reportee) = 0;

    virtual void vif_update(const string& ifname,
			    const string& vifname,
			    const Update& u,
			    bool  is_system_interfaces_reportee) = 0;

    virtual void vifaddr4_update(const string& ifname,
				 const string& vifname,
				 const IPv4&   addr,
				 const Update& u,
				 bool  is_system_interfaces_reportee) = 0;

    virtual void vifaddr6_update(const string& ifname,
				 const string& vifname,
				 const IPv6&   addr,
				 const Update& u,
				 bool  is_system_interfaces_reportee) = 0;

    virtual void updates_completed(bool is_system_interfaces_reportee) = 0;
};

/**
 * @short A class to replicate update notifications to multiple reporters.
 */
class IfConfigUpdateReplicator : public IfConfigUpdateReporterBase {
public:
    typedef IfConfigUpdateReporterBase::Update Update;

public:
    virtual ~IfConfigUpdateReplicator();

    /**
     * Add a reporter instance to update notification list.
     *
     * @return true on success, false if object already receiving updates or
     * could not be added.
     */
    bool add_reporter(IfConfigUpdateReporterBase* rp);

    /**
     * Remove a reporter instance from update notification list.
     *
     * @return true on success, false if instance is not on the
     * receiving updates list.
     */
    bool remove_reporter(IfConfigUpdateReporterBase* rp);

    /**
     * Forward interface update notification to reporter instances on
     * update notification list.
     */
    void interface_update(const string& ifname,
			  const Update& u,
			  bool  is_system_interfaces_reportee);

    /**
     * Forward virtual interface update notification to reporter
     * instances on update notification list.
     */
    void vif_update(const string& ifname,
		    const string& vifname,
		    const Update& u,
		    bool is_system_interfaces_reportee);

    /**
     * Forward virtual interface address update notification to
     * reporter instances on update notification list.
     */
    void vifaddr4_update(const string& ifname,
			 const string& vifname,
			 const IPv4&   addr,
			 const Update& u,
			 bool is_system_interfaces_reportee);

    /**
     * Forward virtual interface address update notification to
     * reporter instances on update notification list.
     */
    void vifaddr6_update(const string& ifname,
			 const string& vifname,
			 const IPv6&   addr,
			 const Update& u,
			 bool is_system_interfaces_reportee);

    /**
     * Forward notification that updates were completed to
     * reporter instances on update notification list.
     */
    void updates_completed(bool is_system_interfaces_reportee);

protected:
    list<IfConfigUpdateReporterBase*> _reporters;
};

/**
 * @short Base class for propagating error information on from IfConfig.
 *
 * When the platform @ref IfConfig updates interfaces it can report
 * errors to an IfConfigErrorReporterBase.  The @ref IfConfig instance
 * takes a pointer to the IfConfigErrorReporterBase object it should use.
 */
class IfConfigErrorReporterBase {
public:
    IfConfigErrorReporterBase() : _error_cnt(0) {}
    virtual ~IfConfigErrorReporterBase() {}

    virtual void config_error(const string& error_msg) = 0;

    virtual void interface_error(const string& ifname,
				 const string& error_msg) = 0;

    virtual void vif_error(const string& ifname,
			   const string& vifname,
			   const string& error_msg) = 0;

    virtual void vifaddr_error(const string& ifname,
			       const string& vifname,
			       const IPv4&   addr,
			       const string& error_msg) = 0;

    virtual void vifaddr_error(const string& ifname,
			       const string& vifname,
			       const IPv6&   addr,
			       const string& error_msg) = 0;

    /**
     * @return error message of first error encountered.
     */
    inline const string& first_error() const	{ return _first_error; }

    /**
     * @return error message of last error encountered.
     */
    inline const string& last_error() const	{ return _last_error; }

    /**
     * @return number of errors reported.
     */
    inline uint32_t error_count() const		{ return _error_cnt; }

    /**
     * Reset error count and error message.
     */
    inline void reset() {
	_first_error.erase();
	_last_error.erase();
	_error_cnt = 0;
    }

    inline void	log_error(const string& s) {
	if (0 == _error_cnt)
	    _first_error = s;
	_last_error = s;
	_error_cnt++;
    }

private:
    string	_last_error;	// last error reported
    string	_first_error;	// first error reported

    uint32_t	_error_cnt;	// number of errors reported
};

/**
 * @short Class for propagating error information during IfConfig operations.
 *
 */
class IfConfigErrorReporter : public IfConfigErrorReporterBase {
public:
    IfConfigErrorReporter();

    void config_error(const string& error_msg);

    void interface_error(const string& ifname,
			 const string& error_msg);

    void vif_error(const string& ifname,
		   const string& vifname,
		   const string& error_msg);

    void vifaddr_error(const string& ifname,
		       const string& vifname,
		       const IPv4&   addr,
		       const string& error_msg);

    void vifaddr_error(const string& ifname,
		       const string& vifname,
		       const IPv6&   addr,
		       const string& error_msg);

private:
};

#endif // __FEA_IFCONFIG_HH__
