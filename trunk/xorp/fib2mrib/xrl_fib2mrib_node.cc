// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/fib2mrib/xrl_fib2mrib_node.cc,v 1.17 2005/02/03 06:51:48 bms Exp $"

#include "fib2mrib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"

#include "fib2mrib_node.hh"
#include "xrl_fib2mrib_node.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"

const TimeVal XrlFib2mribNode::RETRY_TIMEVAL = TimeVal(1, 0);

XrlFib2mribNode::XrlFib2mribNode(EventLoop&	eventloop,
				 const string&	class_name,
				 const string&	finder_hostname,
				 uint16_t	finder_port,
				 const string&	finder_target,
				 const string&	fea_target,
				 const string&	rib_target)
    : Fib2mribNode(eventloop),
      XrlStdRouter(eventloop, class_name.c_str(), finder_hostname.c_str(),
		   finder_port),
      XrlFib2mribTargetBase(&xrl_router()),
      _class_name(xrl_router().class_name()),
      _instance_name(xrl_router().instance_name()),
      _xrl_fea_fti_client(&xrl_router()),
      _xrl_fea_fib_client(&xrl_router()),
      _xrl_rib_client(&xrl_router()),
      _finder_target(finder_target),
      _fea_target(fea_target),
      _rib_target(rib_target),
      _ifmgr(eventloop, fea_target.c_str(), xrl_router().finder_address(),
	     xrl_router().finder_port()),
      _xrl_finder_client(&xrl_router()),
      _is_fea_alive(false),
      _is_fea_registered(false),
      _is_fea_registering(false),
      _is_fea_deregistering(false),
      _is_fea_have_ipv4_tested(false),
      _is_fea_have_ipv6_tested(false),
      _fea_have_ipv4(false),
      _fea_have_ipv6(false),
      _is_fea_fib_client4_registered(false),
      _is_fea_fib_client6_registered(false),
      _is_rib_alive(false),
      _is_rib_registered(false),
      _is_rib_registering(false),
      _is_rib_deregistering(false),
      _is_rib_igp_table4_registered(false),
      _is_rib_igp_table6_registered(false)
{
    _ifmgr.set_observer(dynamic_cast<Fib2mribNode*>(this));
    _ifmgr.attach_hint_observer(dynamic_cast<Fib2mribNode*>(this));
}

XrlFib2mribNode::~XrlFib2mribNode()
{
    Fib2mribNode::shutdown();

    _ifmgr.detach_hint_observer(dynamic_cast<Fib2mribNode*>(this));
    _ifmgr.unset_observer(dynamic_cast<Fib2mribNode*>(this));
}

bool
XrlFib2mribNode::startup()
{
    return Fib2mribNode::startup();
}

bool
XrlFib2mribNode::shutdown()
{
    return Fib2mribNode::shutdown();
}

//
// Finder-related events
//
void
XrlFib2mribNode::finder_disconnect_event()
{
    XLOG_ERROR("Finder disconnect event. Exiting immediately...");

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

//
// Register with the FEA
//
void
XrlFib2mribNode::fea_register_startup()
{
    bool success;

    _fea_register_startup_timer.unschedule();
    _fea_register_shutdown_timer.unschedule();

    if (_is_fea_registered)
	return;		// Already registered

    if (! _is_fea_registering) {
	Fib2mribNode::incr_startup_requests_n();	// XXX: for the ifmgr

	if (! _is_fea_fib_client4_registered)
	    Fib2mribNode::incr_startup_requests_n();
	if (! _is_fea_fib_client6_registered)
	    Fib2mribNode::incr_startup_requests_n();

	_is_fea_registering = true;
    }

    //
    // Register interest in the FEA with the Finder
    //
    success = _xrl_finder_client.send_register_class_event_interest(
	_finder_target.c_str(), _instance_name, _fea_target,
	callback(this, &XrlFib2mribNode::finder_register_interest_fea_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_fea_register_startup_timer =
	    Fib2mribNode::eventloop().new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::fea_register_startup));
	return;
    }
}

void
XrlFib2mribNode::finder_register_interest_fea_cb(
    const XrlError& xrl_error)
{
    // If success, then the FEA birth event will startup the ifmgr
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_registering = false;
	_is_fea_registered = true;
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot register interest in finder events: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // (unless the timer is already running).
    //
    if (! _fea_register_startup_timer.scheduled()) {
	_fea_register_startup_timer =
	    Fib2mribNode::eventloop().new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::fea_register_startup));
    }
}

//
// De-register with the FEA
//
void
XrlFib2mribNode::fea_register_shutdown()
{
    bool success;

    _fea_register_startup_timer.unschedule();
    _fea_register_shutdown_timer.unschedule();

    if (! _is_fea_alive)
	return;		// The FEA is not there anymore

    if (! _is_fea_registered)
	return;		// Not registered

    if (! _is_fea_deregistering) {
	Fib2mribNode::incr_shutdown_requests_n();	// XXX: for the ifmgr

	if (_is_fea_fib_client4_registered)
	    Fib2mribNode::incr_shutdown_requests_n();
	if (_is_fea_fib_client6_registered)
	    Fib2mribNode::incr_shutdown_requests_n();

	_is_fea_deregistering = true;
    }

    //
    // De-register interest in the FEA with the Finder
    //
    success = _xrl_finder_client.send_deregister_class_event_interest(
	_finder_target.c_str(), _instance_name, _fea_target,
	callback(this, &XrlFib2mribNode::finder_deregister_interest_fea_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_fea_register_shutdown_timer =
	    Fib2mribNode::eventloop().new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::fea_register_shutdown));
	return;
    }

    //
    // XXX: when the shutdown is completed, Fib2mribNode::status_change()
    // will be called.
    //
    _ifmgr.shutdown();

    send_fea_delete_fib_client();
}

void
XrlFib2mribNode::finder_deregister_interest_fea_cb(
    const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_deregistering = false;
	_is_fea_registered = false;
	return;
    }

    // TODO: retry de-registration if this was a transport error
    XLOG_ERROR("Failed to deregister interest in finder events: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    _is_fea_deregistering = false;
    _is_fea_registered = false;

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

//
// Register with the RIB
//
void
XrlFib2mribNode::rib_register_startup()
{
    bool success;

    _rib_register_startup_timer.unschedule();
    _rib_register_shutdown_timer.unschedule();

    if (_is_rib_registered)
	return;		// Already registered

    if (! _is_rib_registering) {
	if (! _is_rib_igp_table4_registered)
	    Fib2mribNode::incr_startup_requests_n();
	if (! _is_rib_igp_table6_registered)
	    Fib2mribNode::incr_startup_requests_n();

	_is_rib_registering = true;
    }

    //
    // Register interest in the RIB with the Finder
    //
    success = _xrl_finder_client.send_register_class_event_interest(
	_finder_target.c_str(), _instance_name, _rib_target,
	callback(this, &XrlFib2mribNode::finder_register_interest_rib_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_rib_register_startup_timer =
	    Fib2mribNode::eventloop().new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::rib_register_startup));
	return;
    }
}

void
XrlFib2mribNode::finder_register_interest_rib_cb(
    const XrlError& xrl_error)
{
    // If success, then add the RIB tables
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_registering = false;
	_is_rib_registered = true;
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot register interest in finder events: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // (unless the timer is already running).
    //
    if (! _rib_register_startup_timer.scheduled()) {
	_rib_register_startup_timer =
	    Fib2mribNode::eventloop().new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::rib_register_startup));
    }
}

//
// De-register with the RIB
//
void
XrlFib2mribNode::rib_register_shutdown()
{
    bool success;

    _rib_register_startup_timer.unschedule();
    _rib_register_shutdown_timer.unschedule();

    if (! _is_rib_alive)
	return;		// The RIB is not there anymore

    if (! _is_rib_registered)
	return;		// Not registered

    if (! _is_rib_deregistering) {
	if (_is_rib_igp_table4_registered)
	    Fib2mribNode::incr_shutdown_requests_n();
	if (_is_rib_igp_table6_registered)
	    Fib2mribNode::incr_shutdown_requests_n();

	_is_rib_deregistering = true;
    }

    //
    // De-register interest in the RIB with the Finder
    //
    success = _xrl_finder_client.send_deregister_class_event_interest(
	_finder_target.c_str(), _instance_name, _rib_target,
	callback(this, &XrlFib2mribNode::finder_deregister_interest_rib_cb));

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	_rib_register_shutdown_timer =
	    Fib2mribNode::eventloop().new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::rib_register_shutdown));
	return;
    }

    send_rib_delete_tables();
}

void
XrlFib2mribNode::finder_deregister_interest_rib_cb(
    const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_deregistering = false;
	_is_rib_registered = false;
	return;
    }

    // TODO: retry de-registration if this was a transport error
    XLOG_ERROR("Failed to deregister interest in finder events: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    _is_rib_deregistering = false;
    _is_rib_registered = false;

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

//
// Register as an FEA FIB client
//
void
XrlFib2mribNode::send_fea_add_fib_client()
{
    bool success = true;

    //
    // Test whether the underlying system supports IPv4
    //
    if (! _is_fea_have_ipv4_tested) {
	success = _xrl_fea_fti_client.send_have_ipv4(
	    _fea_target.c_str(),
	    callback(this, &XrlFib2mribNode::fea_fti_client_send_have_ipv4_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to test using the FEA whether the system "
		   "supports IPv4. "
		   "Will try again.");
	goto start_timer_label;
    }

    //
    // Test whether the underlying system supports IPv6
    //
    if (! _is_fea_have_ipv6_tested) {
	success = _xrl_fea_fti_client.send_have_ipv6(
	    _fea_target.c_str(),
	    callback(this, &XrlFib2mribNode::fea_fti_client_send_have_ipv6_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to test using the FEA whether the system "
		   "supports IPv6. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (_fea_have_ipv4 && ! _is_fea_fib_client4_registered) {
	success = _xrl_fea_fib_client.send_add_fib_client4(
	    _fea_target.c_str(),
	    my_xrl_target_name(),
	    true,		/* send_updates */
	    false,		/* send_resolves */
	    callback(this, &XrlFib2mribNode::fea_fib_client_send_add_fib_client4_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to register IPv4 FIB client with the FEA. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (_fea_have_ipv6 && ! _is_fea_fib_client6_registered) {
	success = _xrl_fea_fib_client.send_add_fib_client6(
	    _fea_target.c_str(),
	    my_xrl_target_name(),
	    true,		/* send_updates */
	    false,		/* send_resolves */
	    callback(this, &XrlFib2mribNode::fea_fib_client_send_add_fib_client6_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to register IPv6 FIB client with the FEA. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
    start_timer_label:
	_fea_fib_client_registration_timer =
	    Fib2mribNode::eventloop().new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::send_fea_add_fib_client));
    }
}

void
XrlFib2mribNode::fea_fti_client_send_have_ipv4_cb(const XrlError& xrl_error,
						  const bool* result)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_have_ipv4_tested = true;
	_fea_have_ipv4 = *result;
	send_fea_add_fib_client();
	// XXX: if the underying system doesn't support IPv4, then we are done
	if (! _fea_have_ipv4)
	    Fib2mribNode::decr_startup_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot test using the FEA whether the system "
		   "supports IPv4: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // (unless the timer is already running).
    //
    if (_fea_fib_client_registration_timer.scheduled())
	return;
    _fea_fib_client_registration_timer =
	Fib2mribNode::eventloop().new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::send_fea_add_fib_client));
}

void
XrlFib2mribNode::fea_fti_client_send_have_ipv6_cb(const XrlError& xrl_error,
						  const bool* result)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_have_ipv6_tested = true;
	_fea_have_ipv6 = *result;
	send_fea_add_fib_client();
	// XXX: if the underying system doesn't support IPv6, then we are done
	if (! _fea_have_ipv6)
	    Fib2mribNode::decr_startup_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot test using the FEA whether the system "
		   "supports IPv6: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // (unless the timer is already running).
    //
    if (_fea_fib_client_registration_timer.scheduled())
	return;
    _fea_fib_client_registration_timer =
	Fib2mribNode::eventloop().new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::send_fea_add_fib_client));
}

void
XrlFib2mribNode::fea_fib_client_send_add_fib_client4_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_fib_client4_registered = true;
	send_fea_add_fib_client();
	Fib2mribNode::decr_startup_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot add IPv4 FIB client to the FEA: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // (unless the timer is already running).
    //
    if (_fea_fib_client_registration_timer.scheduled())
	return;
    _fea_fib_client_registration_timer =
	Fib2mribNode::eventloop().new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::send_fea_add_fib_client));
}

void
XrlFib2mribNode::fea_fib_client_send_add_fib_client6_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_fib_client6_registered = true;
	send_fea_add_fib_client();
	Fib2mribNode::decr_startup_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot add IPv6 FIB client to the FEA: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // (unless the timer is already running).
    //
    if (_fea_fib_client_registration_timer.scheduled())
	return;
    _fea_fib_client_registration_timer =
	Fib2mribNode::eventloop().new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::send_fea_add_fib_client));
}

//
// De-register as an FEA FIB client
//
void
XrlFib2mribNode::send_fea_delete_fib_client()
{
    bool success = true;

    if (_is_fea_fib_client4_registered) {
	bool success4;
	success4 = _xrl_fea_fib_client.send_delete_fib_client4(
	    _fea_target.c_str(),
	    my_xrl_target_name(),
	    callback(this, &XrlFib2mribNode::fea_fib_client_send_delete_fib_client4_cb));
	if (success4 != true) {
	    XLOG_ERROR("Failed to deregister IPv4 FIB client with the FEA. "
		"Will give up.");
	    success = false;
	}
    }

    if (_is_fea_fib_client6_registered) {
	bool success6;
	success6 = _xrl_fea_fib_client.send_delete_fib_client6(
	    _fea_target.c_str(),
	    my_xrl_target_name(),
	    callback(this, &XrlFib2mribNode::fea_fib_client_send_delete_fib_client6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to deregister IPv6 FIB client with the FEA. "
		"Will give up.");
	    success = false;
	}
    }

    if (! success) {
	Fib2mribNode::set_status(FAILED);
	Fib2mribNode::update_status();
    }
}

void
XrlFib2mribNode::fea_fib_client_send_delete_fib_client4_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_have_ipv4_tested = false;
	_fea_have_ipv4 = false;
	_is_fea_fib_client4_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	return;
    }

    // TODO: retry de-registration if this was a transport error
    XLOG_ERROR("Failed to deregister IPv4 FIB client with the FEA: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

void
XrlFib2mribNode::fea_fib_client_send_delete_fib_client6_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_have_ipv6_tested = false;
	_fea_have_ipv6 = false;
	_is_fea_fib_client6_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	return;
    }

    // TODO: retry de-registration if this was a transport error
    XLOG_ERROR("Failed to deregister IPv6 FIB client with the FEA: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

//
// Add tables with the RIB
//
void
XrlFib2mribNode::send_rib_add_tables()
{
    bool success = true;

    if (! _is_rib_igp_table4_registered) {
	success = _xrl_rib_client.send_add_igp_table4(
	    _rib_target.c_str(),
	    Fib2mribNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::rib_client_send_add_igp_table4_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to register IPv4 IGP table with the RIB. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (! _is_rib_igp_table6_registered) {
	success = _xrl_rib_client.send_add_igp_table6(
	    _rib_target.c_str(),
	    Fib2mribNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::rib_client_send_add_igp_table6_cb));
	if (success)
	    return;
	XLOG_ERROR("Failed to register IPv6 IGP table with the RIB. "
		   "Will try again.");
	goto start_timer_label;
    }

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
    start_timer_label:
	_rib_igp_table_registration_timer =
	    Fib2mribNode::eventloop().new_oneoff_after(
		RETRY_TIMEVAL,
		callback(this, &XrlFib2mribNode::send_rib_add_tables));
    }
}

void
XrlFib2mribNode::rib_client_send_add_igp_table4_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table4_registered = true;
	send_rib_add_tables();
	Fib2mribNode::decr_startup_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot add IPv4 IGP table to the RIB: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // (unless the timer is already running).
    //
    if (_rib_igp_table_registration_timer.scheduled())
	return;
    _rib_igp_table_registration_timer =
	Fib2mribNode::eventloop().new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::send_rib_add_tables));
}

void
XrlFib2mribNode::rib_client_send_add_igp_table6_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table6_registered = true;
	send_rib_add_tables();
	Fib2mribNode::decr_startup_requests_n();
	return;
    }

    //
    // If a command failed because the other side rejected it, this is fatal.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	XLOG_FATAL("Cannot add IPv4 IGP table to the RIB: %s",
		   xrl_error.str().c_str());
    }

    //
    // If an error, then start a timer to try again
    // (unless the timer is already running).
    //
    if (_rib_igp_table_registration_timer.scheduled())
	return;
    _rib_igp_table_registration_timer =
	Fib2mribNode::eventloop().new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::send_rib_add_tables));
}

//
// Delete tables with the RIB
//
void
XrlFib2mribNode::send_rib_delete_tables()
{
    bool success = true;

    if (_is_rib_igp_table4_registered) {
	bool success4;
	success4 = _xrl_rib_client.send_delete_igp_table4(
	    _rib_target.c_str(),
	    Fib2mribNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::rib_client_send_delete_igp_table4_cb));
	if (success4 != true) {
	    XLOG_ERROR("Failed to deregister IPv4 IGP table with the RIB. "
		"Will give up.");
	    success = false;
	}
    }

    if (_is_rib_igp_table6_registered) {
	bool success6;
	success6 = _xrl_rib_client.send_delete_igp_table6(
	    _rib_target.c_str(),
	    Fib2mribNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::rib_client_send_delete_igp_table6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to deregister IPv6 IGP table with the RIB. "
		"Will give up.");
	    success = false;
	}
    }

    if (! success) {
	Fib2mribNode::set_status(FAILED);
	Fib2mribNode::update_status();
    }
}

void
XrlFib2mribNode::rib_client_send_delete_igp_table4_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table4_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	return;
    }

    // TODO: retry de-registration if this was a transport error
    XLOG_ERROR("Failed to deregister IPv4 IGP table with the RIB: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

void
XrlFib2mribNode::rib_client_send_delete_igp_table6_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table6_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	return;
    }

    // TODO: retry de-registration if this was a transport error
    XLOG_ERROR("Failed to deregister IPv6 IGP table with the RIB: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}


//
// XRL target methods
//

/**
 *  Get name of Xrl Target
 */
XrlCmdError
XrlFib2mribNode::common_0_1_get_target_name(
    // Output values, 
    string&	name)
{
    name = my_xrl_target_name();

    return XrlCmdError::OKAY();
}

/**
 *  Get version string from Xrl Target
 */
XrlCmdError
XrlFib2mribNode::common_0_1_get_version(
    // Output values, 
    string&	version)
{
    version = XORP_MODULE_VERSION;

    return XrlCmdError::OKAY();
}

/**
 *  Get status of Xrl Target
 */
XrlCmdError
XrlFib2mribNode::common_0_1_get_status(
    // Output values, 
    uint32_t&	status, 
    string&	reason)
{
    status = Fib2mribNode::node_status(reason);

    return XrlCmdError::OKAY();
}

/**
 *  Request clean shutdown of Xrl Target
 */
XrlCmdError
XrlFib2mribNode::common_0_1_shutdown()
{
    Fib2mribNode::shutdown();

    return XrlCmdError::OKAY();
}

/**
 *  Handle finder process birth events
 */
XrlCmdError
XrlFib2mribNode::finder_event_observer_0_1_xrl_target_birth(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{
    if (target_class == _fea_target) {
	//
	// XXX: when the startup is completed,
	// IfMgrHintObserver::tree_complete() will be called.
	//
	_is_fea_alive = true;
	if (_ifmgr.startup() != true) {
	    Fib2mribNode::ServiceBase::set_status(FAILED);
	    Fib2mribNode::update_status();
	} else {
	    send_fea_add_fib_client();
	}
    }

    if (target_class == _rib_target) {
	_is_rib_alive = true;
	send_rib_add_tables();
    }

    return XrlCmdError::OKAY();
    UNUSED(target_instance);
}

/**
 *  Handle finder process death events
 */
XrlCmdError
XrlFib2mribNode::finder_event_observer_0_1_xrl_target_death(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{

    bool do_shutdown = false;

    if (_fea_target == target_class) {
	XLOG_ERROR("FEA (instance %s) has died, shutting down.",
		   target_instance.c_str());
	_is_fea_alive = false;
	do_shutdown = true;
    }

    if (_rib_target == target_class) {
	XLOG_ERROR("RIB (instance %s) has died, shutting down.",
		   target_instance.c_str());
	_is_rib_alive = false;
	do_shutdown = true;
    }

    if (do_shutdown)
	Fib2mribNode::shutdown();

    return XrlCmdError::OKAY();
}

/**
 *  Add a route.
 *
 *  @param network the network address prefix of the route to add.
 *
 *  @param nexthop the address of the next-hop router toward the
 *  destination.
 *
 *  @param ifname the name of the physical interface toward the
 *  destination.
 *
 *  @param vifname the name of the virtual interface toward the
 *  destination.
 *
 *  @param metric the routing metric toward the destination.
 *
 *  @param admin_distance the administratively defined distance toward the
 *  destination.
 *
 *  @param protocol_origin the name of the protocol that originated this
 *  route.
 *
 *  @param xorp_route true if this route was installed by XORP.
 */
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_add_route4(
    // Input values,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&		xorp_route)
{
    string error_msg;

    if (Fib2mribNode::add_route4(network, nexthop, ifname, vifname,
				 metric, admin_distance,
				 protocol_origin, xorp_route,
				 error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_add_route6(
    // Input values,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&	xorp_route)
{
    string error_msg;

    if (Fib2mribNode::add_route6(network, nexthop, ifname, vifname,
				 metric, admin_distance,
				 protocol_origin, xorp_route,
				 error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Replace a route.
 *
 *  @param network the network address prefix of the route to replace.
 *
 *  @param nexthop the address of the next-hop router toward the
 *  destination.
 *
 *  @param ifname the name of the physical interface toward the
 *  destination.
 *
 *  @param vifname the name of the virtual interface toward the
 *  destination.
 *
 *  @param metric the routing metric toward the destination.
 *
 *  @param admin_distance the administratively defined distance toward the
 *  destination.
 *
 *  @param protocol_origin the name of the protocol that originated this
 *  route.
 *
 *  @param xorp_route true if this route was installed by XORP.
 */
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_replace_route4(
    // Input values,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&		xorp_route)
{
    string error_msg;

    if (Fib2mribNode::replace_route4(network, nexthop, ifname, vifname,
				     metric, admin_distance,
				     protocol_origin, xorp_route,
				     error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_replace_route6(
    // Input values,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&	xorp_route)
{
    string error_msg;

    if (Fib2mribNode::replace_route6(network, nexthop, ifname, vifname,
				     metric, admin_distance,
				     protocol_origin, xorp_route,
				     error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Delete a route.
 *
 *  @param network the network address prefix of the route to delete.
 *
 *  @param ifname the name of the physical interface toward the
 *  destination.
 *
 *  @param vifname the name of the virtual interface toward the
 */
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_delete_route4(
    // Input values,
    const IPv4Net&	network,
    const string&	ifname,
    const string&	vifname)
{
    string error_msg;

    if (Fib2mribNode::delete_route4(network, ifname, vifname, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_delete_route6(
    // Input values,
    const IPv6Net&	network,
    const string&	ifname,
    const string&	vifname)
{
    string error_msg;

    if (Fib2mribNode::delete_route6(network, ifname, vifname, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Route resolve notification.
 *
 *  @param network the network address prefix of the lookup
 *  which failed or for which upper layer intervention is
 *  requested from the FIB.
 */
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_resolve_route4(
    // Input values,
    const IPv4Net&	network)
{
    UNUSED(network);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_resolve_route6(
    // Input values,
    const IPv6Net&	network)
{
    UNUSED(network);
    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable/start/stop Fib2mrib.
 *
 *  @param enable if true, then enable Fib2mrib, otherwise disable it.
 */
XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_enable_fib2mrib(
    // Input values,
    const bool&	enable)
{
    UNUSED(enable);

    // XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_start_fib2mrib()
{
    // XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_stop_fib2mrib()
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable the Fib2mrib trace log for all operations.
 *
 *  @param enable if true, then enable the trace log, otherwise disable it.
 */
XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_enable_log_trace_all(
    // Input values,
    const bool&	enable)
{
    Fib2mribNode::set_log_trace(enable);

    return XrlCmdError::OKAY();
}

/**
 * Inform the RIB about a route change.
 *
 * @param fib2mrib_route the route with the information about the change.
 */
void
XrlFib2mribNode::inform_rib_route_change(const Fib2mribRoute& fib2mrib_route)
{
    // Add the request to the queue
    _inform_rib_queue.push_back(fib2mrib_route);

    // If the queue was empty before, start sending the routes
    if (_inform_rib_queue.size() == 1) {
	send_rib_route_change();
    }
}

/**
 * Cancel a pending request to inform the RIB about a route change.
 *
 * @param fib2mrib_route the route with the request that would be canceled.
 */
void
XrlFib2mribNode::cancel_rib_route_change(const Fib2mribRoute& fib2mrib_route)
{
    list<Fib2mribRoute>::iterator iter;

    for (iter = _inform_rib_queue.begin();
	 iter != _inform_rib_queue.end();
	 ++iter) {
	Fib2mribRoute& tmp_fib2mrib_route = *iter;
	if (tmp_fib2mrib_route == fib2mrib_route)
	    tmp_fib2mrib_route.set_ignored(true);
    }
}

void
XrlFib2mribNode::send_rib_route_change()
{
    bool success = true;

    do {
	// Pop-up all routes that are to be ignored
	if (_inform_rib_queue.empty())
	    return;		// No more route changes to send

	Fib2mribRoute& tmp_fib2mrib_route = _inform_rib_queue.front();
	if (tmp_fib2mrib_route.is_ignored()) {
	    _inform_rib_queue.pop_front();
	    continue;
	}
	break;
    } while (true);

    Fib2mribRoute& fib2mrib_route = _inform_rib_queue.front();

    //
    // Check whether we have already registered with the RIB
    //
    if (fib2mrib_route.is_ipv4() && (! _is_rib_igp_table4_registered)) {
	success = false;
	goto start_timer_label;
    }

    if (fib2mrib_route.is_ipv6() && (! _is_rib_igp_table6_registered)) {
	success = false;
	goto start_timer_label;
    }

    //
    // Send the appropriate XRL
    //
    if (fib2mrib_route.is_add_route()) {
	if (fib2mrib_route.is_ipv4()) {
	    if (fib2mrib_route.is_interface_route()) {
		success = _xrl_rib_client.send_add_interface_route4(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv4net(),
		    fib2mrib_route.nexthop().get_ipv4(),
		    fib2mrib_route.ifname(),
		    fib2mrib_route.vifname(),
		    fib2mrib_route.metric(),
		    XrlAtomList(),		// XXX: no policy
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_add_route4(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv4net(),
		    fib2mrib_route.nexthop().get_ipv4(),
		    fib2mrib_route.metric(),
		    XrlAtomList(),		// XXX: no policy
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
	if (fib2mrib_route.is_ipv6()) {
	    if (fib2mrib_route.is_interface_route()) {
		success = _xrl_rib_client.send_add_interface_route6(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv6net(),
		    fib2mrib_route.nexthop().get_ipv6(),
		    fib2mrib_route.ifname(),
		    fib2mrib_route.vifname(),
		    fib2mrib_route.metric(),
		    XrlAtomList(),		// XXX: no policy
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_add_route6(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv6net(),
		    fib2mrib_route.nexthop().get_ipv6(),
		    fib2mrib_route.metric(),
		    XrlAtomList(),		// XXX: no policy
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
    }

    if (fib2mrib_route.is_replace_route()) {
	if (fib2mrib_route.is_ipv4()) {
	    if (fib2mrib_route.is_interface_route()) {
		success = _xrl_rib_client.send_replace_interface_route4(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv4net(),
		    fib2mrib_route.nexthop().get_ipv4(),
		    fib2mrib_route.ifname(),
		    fib2mrib_route.vifname(),
		    fib2mrib_route.metric(),
		    XrlAtomList(),		// XXX: no policy
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_replace_route4(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv4net(),
		    fib2mrib_route.nexthop().get_ipv4(),
		    fib2mrib_route.metric(),
		    XrlAtomList(),		// XXX: no policy
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
	if (fib2mrib_route.is_ipv6()) {
	    if (fib2mrib_route.is_interface_route()) {
		success = _xrl_rib_client.send_replace_interface_route6(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv6net(),
		    fib2mrib_route.nexthop().get_ipv6(),
		    fib2mrib_route.ifname(),
		    fib2mrib_route.vifname(),
		    fib2mrib_route.metric(),
		    XrlAtomList(),		// XXX: no policy
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    } else {
		success = _xrl_rib_client.send_replace_route6(
		    _rib_target.c_str(),
		    Fib2mribNode::protocol_name(),
		    false,			/* unicast */
		    true,			/* multicast */
		    fib2mrib_route.network().get_ipv6net(),
		    fib2mrib_route.nexthop().get_ipv6(),
		    fib2mrib_route.metric(),
		    XrlAtomList(),		// XXX: no policy
		    callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
		if (success)
		    return;
	    }
	}
    }

    if (fib2mrib_route.is_delete_route()) {
	if (fib2mrib_route.is_ipv4()) {
	    success = _xrl_rib_client.send_delete_route4(
		_rib_target.c_str(),
		Fib2mribNode::protocol_name(),
		false,			/* unicast */
		true,			/* multicast */
		fib2mrib_route.network().get_ipv4net(),
		callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
	    if (success)
		return;
	}
	if (fib2mrib_route.is_ipv6()) {
	    success = _xrl_rib_client.send_delete_route6(
		_rib_target.c_str(),
		Fib2mribNode::protocol_name(),
		false,			/* unicast */
		true,			/* multicast */
		fib2mrib_route.network().get_ipv6net(),
		callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
	    if (success)
		return;
	}
    }

    if (! success) {
	//
	// If an error, then start a timer to try again.
	//
	XLOG_ERROR("Failed to %s route for %s with the RIB. "
		   "Will try again.",
		   (fib2mrib_route.is_add_route())? "add"
		   : (fib2mrib_route.is_replace_route())? "replace"
		   : "delete",
		   fib2mrib_route.network().str().c_str());
    start_timer_label:
	_inform_rib_queue_timer = Fib2mribNode::eventloop().new_oneoff_after(
	    RETRY_TIMEVAL,
	    callback(this, &XrlFib2mribNode::send_rib_route_change));
    }
}

void
XrlFib2mribNode::send_rib_route_change_cb(const XrlError& xrl_error)
{
    // If success, then send the next route change
    if (xrl_error == XrlError::OKAY()) {
	_inform_rib_queue.pop_front();
	send_rib_route_change();
	return;
    }

    //
    // If a command failed because the other side rejected it,
    // then send the next route change.
    //
    if (xrl_error == XrlError::COMMAND_FAILED()) {
	_inform_rib_queue.pop_front();
	send_rib_route_change();
	return;
    }

    //
    // If an error, then start a timer to try again
    // (unless the timer is already running).
    //
    if (_inform_rib_queue_timer.scheduled())
	return;
    _inform_rib_queue_timer = Fib2mribNode::eventloop().new_oneoff_after(
	RETRY_TIMEVAL,
	callback(this, &XrlFib2mribNode::send_rib_route_change));
}
