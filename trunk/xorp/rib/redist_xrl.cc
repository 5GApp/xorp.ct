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

#ident "$XORP: xorp/rib/redist_xrl.cc,v 1.23 2006/03/16 00:05:30 pavlin Exp $"

#include <list>
#include <string>

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/safe_callback_obj.hh"

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/redist4_xif.hh"
#include "xrl/interfaces/redist6_xif.hh"
#include "xrl/interfaces/redist_transaction4_xif.hh"
#include "xrl/interfaces/redist_transaction6_xif.hh"

#include "route.hh"
#include "redist_xrl.hh"
#include "profile_vars.hh"

/**
 * Base class for RedistXrlOutput Tasks.  Classes derived from this
 * store enough state to dispatch XRL, or other task, at some
 * subsequent time in the future.
 */
template <typename A>
class RedistXrlTask : public CallbackSafeObject
{
public:
    RedistXrlTask(RedistXrlOutput<A>* parent)
	: _parent(parent), _attempts(0)
    {}
    virtual ~RedistXrlTask() {}

    /**
     * @return true on success, false if XrlRouter could not dispatch
     * request.
     */
    virtual bool dispatch(XrlRouter& xrl_router, Profile& profile) = 0;

    /**
     * Get number of times dispatch() invoked on instance.
     */
    inline uint32_t dispatch_attempts() const		{ return _attempts; }

protected:
    inline void incr_dispatch_attempts()		{ _attempts++; }
    inline RedistXrlOutput<A>* parent()			{ return _parent; }
    inline const RedistXrlOutput<A>* parent() const	{ return _parent; }

    inline void signal_complete_ok()	{ _parent->task_completed(this); }
    inline void signal_fatal_failure()	{ _parent->task_failed_fatally(this); }

private:
    RedistXrlOutput<A>* _parent;
    uint32_t		_attempts;
};


// ----------------------------------------------------------------------------
// Task declarations

template <typename A>
class AddRoute : public RedistXrlTask<A>
{
public:
    AddRoute(RedistXrlOutput<A>* parent, const IPRouteEntry<A>& ipr);
    virtual bool dispatch(XrlRouter& xrl_router, Profile& profile);
    void dispatch_complete(const XrlError& xe);
protected:
    IPNet<A>	_net;
    A		_nexthop;
    string	_ifname;
    string	_vifname;
    uint32_t	_metric;
    uint32_t	_admin_distance;
    string	_protocol_origin;
};

template <typename A>
class DeleteRoute : public RedistXrlTask<A>
{
public:
    DeleteRoute(RedistXrlOutput<A>* parent, const IPRouteEntry<A>& ipr);
    virtual bool dispatch(XrlRouter& xrl_router, Profile& profile);
    void dispatch_complete(const XrlError& xe);
protected:
    IPNet<A>	_net;
    A		_nexthop;
    string	_ifname;
    string	_vifname;
    uint32_t	_metric;
    uint32_t	_admin_distance;
    string	_protocol_origin;
};

template <typename A>
class StartingRouteDump : public RedistXrlTask<A>
{
public:
    StartingRouteDump(RedistXrlOutput<A>* parent);
    virtual bool dispatch(XrlRouter& xrl_router, Profile& profile);
    void dispatch_complete(const XrlError& xe);
};

template <typename A>
class FinishingRouteDump : public RedistXrlTask<A>
{
public:
    FinishingRouteDump(RedistXrlOutput<A>* parent);
    virtual bool dispatch(XrlRouter& xrl_router, Profile& profile);
    void dispatch_complete(const XrlError& xe);
};

template <typename A>
class Pause : public RedistXrlTask<A>
{
public:
    Pause(RedistXrlOutput<A>* parent, uint32_t ms);
    virtual bool dispatch(XrlRouter&  xrl_router, Profile& profile);
    void expire();
private:
    XorpTimer _t;
    uint32_t  _p_ms;
};


// ----------------------------------------------------------------------------
// AddRoute implementation

template <typename A>
AddRoute<A>::AddRoute(RedistXrlOutput<A>* parent, const IPRouteEntry<A>& ipr)
    : RedistXrlTask<A>(parent),
      _net(ipr.net()),
      _nexthop(ipr.nexthop_addr()),
      _ifname(ipr.vif()->ifname()),
      _vifname(ipr.vif()->name()),
      _metric(ipr.metric()),
      _admin_distance(ipr.admin_distance()),
      _protocol_origin(ipr.protocol().name())
{
}

template <>
bool
AddRoute<IPv4>::dispatch(XrlRouter& xrl_router, Profile& profile)
{
    if (profile.enabled(profile_route_rpc_out))
	profile.log(profile_route_rpc_out,
		    c_format("add %s", _net.str().c_str()));

    RedistXrlOutput<IPv4>* p = this->parent();

    XrlRedist4V0p1Client cl(&xrl_router);
    return cl.send_add_route(p->xrl_target_name().c_str(),
			     _net, _nexthop, _ifname, _vifname, _metric,
			     _admin_distance, p->cookie(),
			     _protocol_origin,
			     callback(this, &AddRoute<IPv4>::dispatch_complete)
	);
}

template <>
bool
AddRoute<IPv6>::dispatch(XrlRouter& xrl_router, Profile& profile)
{
    if (profile.enabled(profile_route_rpc_out))
	profile.log(profile_route_rpc_out,
		    c_format("add %s", _net.str().c_str()));

    RedistXrlOutput<IPv6>* p = this->parent();

    XrlRedist6V0p1Client cl(&xrl_router);
    return cl.send_add_route(p->xrl_target_name().c_str(),
			     _net, _nexthop, _ifname, _vifname, _metric,
			     _admin_distance, p->cookie(),
			     _protocol_origin,
			     callback(this, &AddRoute<IPv6>::dispatch_complete)
	);

}

template <typename A>
void
AddRoute<A>::dispatch_complete(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	this->signal_complete_ok();
	return;
    } else if (xe == XrlError::COMMAND_FAILED()) {
	XLOG_ERROR("Failed to redistribute route add for %s: %s",
		   _net.str().c_str(),
		   xe.str().c_str());
	this->signal_complete_ok();
	return;
    }
    // For now all errors are signalled fatal
    XLOG_ERROR("Fatal error during route redistribution: %s",
	       xe.str().c_str());

    this->signal_fatal_failure();
}


// ----------------------------------------------------------------------------
// DeleteRoute implementation

template <typename A>
DeleteRoute<A>::DeleteRoute(RedistXrlOutput<A>* parent,
			    const IPRouteEntry<A>& ipr)
    : RedistXrlTask<A>(parent),
      _net(ipr.net()),
      _nexthop(ipr.nexthop_addr()),
      _ifname(ipr.vif()->ifname()),
      _vifname(ipr.vif()->name()),
      _metric(ipr.metric()),
      _admin_distance(ipr.admin_distance()),
      _protocol_origin(ipr.protocol().name())
{
}

template <>
bool
DeleteRoute<IPv4>::dispatch(XrlRouter& xrl_router, Profile& profile)
{
    if (profile.enabled(profile_route_rpc_out))
	profile.log(profile_route_rpc_out,
		    c_format("delete %s", _net.str().c_str()));

    RedistXrlOutput<IPv4>* p = this->parent();

    XrlRedist4V0p1Client cl(&xrl_router);
    return cl.send_delete_route(p->xrl_target_name().c_str(),
				_net, _nexthop, _ifname, _vifname, _metric,
				_admin_distance, p->cookie(),
				_protocol_origin,
				callback(this,
					 &DeleteRoute<IPv4>::dispatch_complete)
	);
}

template <>
bool
DeleteRoute<IPv6>::dispatch(XrlRouter& xrl_router, Profile& profile)
{
    if (profile.enabled(profile_route_rpc_out))
	profile.log(profile_route_rpc_out,
		    c_format("delete %s", _net.str().c_str()));

    RedistXrlOutput<IPv6>* p = this->parent();

    XrlRedist6V0p1Client cl(&xrl_router);
    return cl.send_delete_route(p->xrl_target_name().c_str(),
				_net, _nexthop, _ifname, _vifname, _metric,
				_admin_distance, p->cookie(),
				_protocol_origin,
				callback(this,
					 &DeleteRoute<IPv6>::dispatch_complete)
	);
}

template <typename A>
void
DeleteRoute<A>::dispatch_complete(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	this->signal_complete_ok();
	return;
    } else if (xe == XrlError::COMMAND_FAILED()) {
	XLOG_ERROR("Failed to redistribute route delete for %s: %s",
		   _net.str().c_str(),
		   xe.str().c_str());
	this->signal_complete_ok();
	return;
    }
    // XXX For now all errors signalled as fatal
    XLOG_ERROR("Fatal error during route redistribution: %s",
	       xe.str().c_str());
    this->signal_fatal_failure();
}


// ----------------------------------------------------------------------------
// StartingRouteDump implementation

template <typename A>
StartingRouteDump<A>::StartingRouteDump(RedistXrlOutput<A>* parent)
    : RedistXrlTask<A>(parent)
{
}

template <>
bool
StartingRouteDump<IPv4>::dispatch(XrlRouter& xrl_router, Profile&)
{
    RedistXrlOutput<IPv4>* p = this->parent();

    XrlRedist4V0p1Client cl(&xrl_router);
    return cl.send_starting_route_dump(
		p->xrl_target_name().c_str(),
		p->cookie(),
		callback(this, &StartingRouteDump<IPv4>::dispatch_complete)
		);
}

template <>
bool
StartingRouteDump<IPv6>::dispatch(XrlRouter& xrl_router, Profile&)
{
    RedistXrlOutput<IPv6>* p = this->parent();

    XrlRedist6V0p1Client cl(&xrl_router);
    return cl.send_starting_route_dump(
		p->xrl_target_name().c_str(),
		p->cookie(),
		callback(this, &StartingRouteDump<IPv6>::dispatch_complete)
		);
}

template <typename A>
void
StartingRouteDump<A>::dispatch_complete(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	this->signal_complete_ok();
	return;
    } else if (xe == XrlError::COMMAND_FAILED()) {
	XLOG_ERROR("Failed to send starting route dump: %s",
		   xe.str().c_str());
	this->signal_complete_ok();
	return;
    }
    // XXX For now all errors signalled as fatal
    XLOG_ERROR("Fatal error during route redistribution: %s",
	       xe.str().c_str());
    this->signal_fatal_failure();
}


// ----------------------------------------------------------------------------
// FinishingRouteDump implementation

template <typename A>
FinishingRouteDump<A>::FinishingRouteDump(RedistXrlOutput<A>* parent)
    : RedistXrlTask<A>(parent)
{
}

template <>
bool
FinishingRouteDump<IPv4>::dispatch(XrlRouter& xrl_router, Profile&)
{
    RedistXrlOutput<IPv4>* p = this->parent();

    XrlRedist4V0p1Client cl(&xrl_router);
    return cl.send_finishing_route_dump(
		p->xrl_target_name().c_str(),
		p->cookie(),
		callback(this, &FinishingRouteDump<IPv4>::dispatch_complete)
		);
}

template <>
bool
FinishingRouteDump<IPv6>::dispatch(XrlRouter& xrl_router, Profile&)
{
    RedistXrlOutput<IPv6>* p = this->parent();

    XrlRedist6V0p1Client cl(&xrl_router);
    return cl.send_finishing_route_dump(
		p->xrl_target_name().c_str(),
		p->cookie(),
		callback(this, &FinishingRouteDump<IPv6>::dispatch_complete)
		);
}

template <typename A>
void
FinishingRouteDump<A>::dispatch_complete(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	this->signal_complete_ok();
	return;
    } else if (xe == XrlError::COMMAND_FAILED()) {
	XLOG_ERROR("Failed to send finishing route dump: %s",
		   xe.str().c_str());
	this->signal_complete_ok();
	return;
    }
    // XXX For now all errors signalled as fatal
    XLOG_ERROR("Fatal error during route redistribution: %s",
	       xe.str().c_str());
    this->signal_fatal_failure();
}


// ----------------------------------------------------------------------------
// Pause implementation

template <typename A>
Pause<A>::Pause(RedistXrlOutput<A>* parent, uint32_t ms)
    : RedistXrlTask<A>(parent), _p_ms(ms)
{
}

template <typename A>
bool
Pause<A>::dispatch(XrlRouter& xrl_router, Profile&)
{
    this->incr_dispatch_attempts();
    EventLoop& e = xrl_router.eventloop();
    _t = e.new_oneoff_after_ms(_p_ms, callback(this, &Pause<A>::expire));
    return true;
}

template <typename A>
void
Pause<A>::expire()
{
    this->signal_complete_ok();
}


// ----------------------------------------------------------------------------
// RedistXrlOutput implementation

template <typename A>
RedistXrlOutput<A>::RedistXrlOutput(Redistributor<A>*	redistributor,
				    XrlRouter&		xrl_router,
				    Profile&		profile,
				    const string&	from_protocol,
				    const string&	xrl_target_name,
				    const IPNet<A>&	network_prefix,
				    const string&	cookie)
    : RedistOutput<A>(redistributor), _xrl_router(xrl_router), 
      _profile(profile),
      _from_protocol(from_protocol), _target_name(xrl_target_name),
      _network_prefix(network_prefix), _cookie(cookie), _n_tasks(0)
{
}

template <typename A>
RedistXrlOutput<A>::~RedistXrlOutput()
{
    while (_tasks.empty() == false) {
	delete _tasks.front();
	_tasks.pop_front();
    }
    _n_tasks = 0;
}

template <typename A>
void
RedistXrlOutput<A>::incr_task_count()
{
    _n_tasks++;
    if (_n_tasks == HI_WATER) {
	this->announce_high_water();
    }
}

template <typename A>
void
RedistXrlOutput<A>::decr_task_count()
{
    _n_tasks--;
    if (_n_tasks == LO_WATER - 1) {
	this->announce_low_water();
    }
}

template <typename A>
uint32_t
RedistXrlOutput<A>::task_count() const
{
    return _n_tasks;
}

template <typename A>
void
RedistXrlOutput<A>::enqueue_task(Task* task)
{
    _tasks.push_back(task);
    incr_task_count();
}

template <typename A>
void
RedistXrlOutput<A>::dequeue_task(Task* task)
{
    XLOG_ASSERT(task == _tasks.front());
    delete _tasks.front();
    _tasks.pop_front();
    decr_task_count();
}

template <typename A>
void
RedistXrlOutput<A>::add_route(const IPRouteEntry<A>& ipr)
{
    if (! _network_prefix.contains(ipr.net()))
	return;		// The target is not interested in this route

    if (_profile.enabled(profile_route_rpc_in))
	_profile.log(profile_route_rpc_in,
		     c_format("add %s", ipr.net().str().c_str()));
    
    enqueue_task(new AddRoute<A>(this, ipr));
    if (task_count() == 1) {
	start_running_tasks();
    }
}

template <typename A>
void
RedistXrlOutput<A>::delete_route(const IPRouteEntry<A>& ipr)
{
    if (! _network_prefix.contains(ipr.net()))
	return;		// The target is not interested in this route

    if (_profile.enabled(profile_route_rpc_in))
	_profile.log(profile_route_rpc_in,
		     c_format("delete %s", ipr.net().str().c_str()));

    enqueue_task(new DeleteRoute<A>(this, ipr));
    if (task_count() == 1) {
	start_running_tasks();
    }
}

template <typename A>
void
RedistXrlOutput<A>::starting_route_dump()
{
    enqueue_task(new StartingRouteDump<A>(this));
    if (task_count() == 1) {
	start_running_tasks();
    }
}

template <typename A>
void
RedistXrlOutput<A>::finishing_route_dump()
{
    enqueue_task(new FinishingRouteDump<A>(this));
    if (task_count() == 1) {
	start_running_tasks();
    }
}

template <typename A>
void
RedistXrlOutput<A>::start_running_tasks()
{
    XLOG_ASSERT(task_count() == 1);
    start_next_task();
}

template <typename A>
void
RedistXrlOutput<A>::start_next_task()
{
    XLOG_ASSERT(task_count() >= 1);
    RedistXrlTask<A>* t = _tasks.front();

    if (t->dispatch(_xrl_router, _profile) == false) {
	if (t->dispatch_attempts() > MAX_RETRIES) {
	    XLOG_ERROR("Failed to dispatch command after %u attempts.",
		       XORP_UINT_CAST(t->dispatch_attempts()));
	    // XXX signal failure
	}
	// Dispatch of task failed.  XrlRouter is presumeably backlogged.
	// Insert a delay and dispatch that to cause later attempt at failing
	// task.
	t = new Pause<A>(this, RETRY_PAUSE_MS);
	_tasks.push_front(t);
	incr_task_count();
	t->dispatch(_xrl_router, _profile);
    }
}

template <typename A>
void
RedistXrlOutput<A>::task_completed(RedistXrlTask<A>* task)
{
    dequeue_task(task);
    if (task_count() != 0) {
	start_next_task();
	return;
    }
}

template <typename A>
void
RedistXrlOutput<A>::task_failed_fatally(RedistXrlTask<A>* task)
{
    XLOG_ASSERT(_tasks.front() == task);
    delete _tasks.front();
    _tasks.pop_front();
    decr_task_count();
    this->announce_fatal_error();
}


// ----------------------------------------------------------------------------
// RedistTransactionXrlOutput Commands

template <typename A>
class AddTransactionRoute : public AddRoute<A> {
public:
    AddTransactionRoute(RedistTransactionXrlOutput<A>* parent,
			const IPRouteEntry<A>& ipr)
	: AddRoute<A>(parent, ipr) {
	parent->incr_transaction_size();
    }
    virtual bool dispatch(XrlRouter& xrl_router, Profile& profile);
};

template <typename A>
class DeleteTransactionRoute : public DeleteRoute<A> {
public:
    DeleteTransactionRoute(RedistTransactionXrlOutput<A>* parent,
			   const IPRouteEntry<A>& ipr)
	: DeleteRoute<A>(parent, ipr) {
	parent->incr_transaction_size();
    }
    virtual bool dispatch(XrlRouter& xrl_router, Profile& profile);
};

template <typename A>
class StartTransaction : public RedistXrlTask<A> {
public:
    StartTransaction(RedistTransactionXrlOutput<A>* parent)
	: RedistXrlTask<A>(parent) {
	parent->reset_transaction_size();
    }
    virtual bool dispatch(XrlRouter&  xrl_router, Profile& profile);
    void dispatch_complete(const XrlError& xe, const uint32_t* tid);
};

template <typename A>
class CommitTransaction : public RedistXrlTask<A> {
public:
    CommitTransaction(RedistTransactionXrlOutput<A>* parent)
	: RedistXrlTask<A>(parent) {
	parent->reset_transaction_size();
    }
    virtual bool dispatch(XrlRouter&  xrl_router, Profile& profile);
    void dispatch_complete(const XrlError& xe);
};

template <typename A>
class AbortTransaction : public RedistXrlTask<A> {
public:
    AbortTransaction(RedistTransactionXrlOutput<A>* parent)
	: RedistXrlTask<A>(parent) {
	parent->reset_transaction_size();
    }
    virtual bool dispatch(XrlRouter&  xrl_router, Profile& profile);
    void dispatch_complete(const XrlError& xe);
};


// ----------------------------------------------------------------------------
// AddTransactionRoute implementation

template <>
bool
AddTransactionRoute<IPv4>::dispatch(XrlRouter& xrl_router, Profile& profile)
{
    RedistTransactionXrlOutput<IPv4>* p =
	reinterpret_cast<RedistTransactionXrlOutput<IPv4>*>(this->parent());

    if (p->transaction_in_error() || ! p->transaction_in_progress()) {
	XLOG_ERROR("Transaction error: failed to redistribute "
		   "route add for %s", _net.str().c_str());
	this->signal_complete_ok();
	return true;	// XXX: we return true to avoid retransmission
    }

    if (profile.enabled(profile_route_rpc_out))
	profile.log(profile_route_rpc_out,
		     c_format("add %s %s %s %u", 
			      p->xrl_target_name().c_str(),
			      _net.str().c_str(),
			      _nexthop.str().c_str(),
			      XORP_UINT_CAST(_metric)));

    XrlRedistTransaction4V0p1Client cl(&xrl_router);
    return cl.send_add_route(p->xrl_target_name().c_str(),
			     p->tid(),
			     _net, _nexthop, _ifname, _vifname, _metric,
			     _admin_distance, p->cookie(),
			     _protocol_origin,
			     callback(static_cast<AddRoute<IPv4>*>(this),
				      &AddRoute<IPv4>::dispatch_complete)
	);
}

template <>
bool
AddTransactionRoute<IPv6>::dispatch(XrlRouter& xrl_router, Profile& profile)
{
    RedistTransactionXrlOutput<IPv6>* p =
	reinterpret_cast<RedistTransactionXrlOutput<IPv6>*>(this->parent());

    if (p->transaction_in_error() || ! p->transaction_in_progress()) {
	XLOG_ERROR("Transaction error: failed to redistribute "
		   "route add for %s", _net.str().c_str());
	this->signal_complete_ok();
	return true;	// XXX: we return true to avoid retransmission
    }

    if (profile.enabled(profile_route_rpc_out))
	profile.log(profile_route_rpc_out,
		     c_format("add %s %s %s %u", 
			      p->xrl_target_name().c_str(),
			      _net.str().c_str(),
			      _nexthop.str().c_str(),
			      XORP_UINT_CAST(_metric)));

    XrlRedistTransaction6V0p1Client cl(&xrl_router);
    return cl.send_add_route(p->xrl_target_name().c_str(),
			     p->tid(),
			     _net, _nexthop, _ifname, _vifname, _metric,
			     _admin_distance, p->cookie(),
			     _protocol_origin,
			     callback(static_cast<AddRoute<IPv6>*>(this),
				      &AddRoute<IPv6>::dispatch_complete)
	);
}


// ----------------------------------------------------------------------------
// DeleteTransactionRoute implementation

template <>
bool
DeleteTransactionRoute<IPv4>::dispatch(XrlRouter& xrl_router, Profile& profile)
{
    RedistTransactionXrlOutput<IPv4>* p =
	reinterpret_cast<RedistTransactionXrlOutput<IPv4>*>(this->parent());

    if (p->transaction_in_error() || ! p->transaction_in_progress()) {
	XLOG_ERROR("Transaction error: failed to redistribute "
		   "route delete for %s", _net.str().c_str());
	this->signal_complete_ok();
	return true;	// XXX: we return true to avoid retransmission
    }

    if (profile.enabled(profile_route_rpc_out))
	profile.log(profile_route_rpc_out,
		     c_format("delete %s %s", 
			      p->xrl_target_name().c_str(),
			      _net.str().c_str()));

    XrlRedistTransaction4V0p1Client cl(&xrl_router);
    return cl.send_delete_route(p->xrl_target_name().c_str(),
				p->tid(),
				_net, _nexthop, _ifname, _vifname, _metric,
				_admin_distance, p->cookie(),
				_protocol_origin,
				callback(static_cast<DeleteRoute<IPv4>*>(this),
					 &DeleteRoute<IPv4>::dispatch_complete)
	);
}

template <>
bool
DeleteTransactionRoute<IPv6>::dispatch(XrlRouter& xrl_router, Profile& profile)
{
    RedistTransactionXrlOutput<IPv6>* p =
	reinterpret_cast<RedistTransactionXrlOutput<IPv6>*>(this->parent());

    if (p->transaction_in_error() || ! p->transaction_in_progress()) {
	XLOG_ERROR("Transaction error: failed to redistribute "
		   "route delete for %s", _net.str().c_str());
	this->signal_complete_ok();
	return true;	// XXX: we return true to avoid retransmission
    }

    if (profile.enabled(profile_route_rpc_out))
	profile.log(profile_route_rpc_out,
		     c_format("delete %s %s", 
			      p->xrl_target_name().c_str(),
			      _net.str().c_str()));

    XrlRedistTransaction6V0p1Client cl(&xrl_router);
    return cl.send_delete_route(p->xrl_target_name().c_str(),
				p->tid(),
				_net, _nexthop, _ifname, _vifname, _metric,
				_admin_distance, p->cookie(),
				_protocol_origin,
				callback(static_cast<DeleteRoute<IPv6>*>(this),
					 &DeleteRoute<IPv6>::dispatch_complete)
	);
}


// ----------------------------------------------------------------------------
// StartTransaction implementation

template <>
bool
StartTransaction<IPv4>::dispatch(XrlRouter& xrl_router, Profile&)
{
    RedistTransactionXrlOutput<IPv4>* p =
	reinterpret_cast<RedistTransactionXrlOutput<IPv4>*>(this->parent());

    p->set_tid(0);
    p->set_transaction_in_progress(true);
    p->set_transaction_in_error(false);

    XrlRedistTransaction4V0p1Client cl(&xrl_router);
    return cl.send_start_transaction(
	p->xrl_target_name().c_str(),
	callback(this, &StartTransaction<IPv4>::dispatch_complete));
}

template <>
bool
StartTransaction<IPv6>::dispatch(XrlRouter& xrl_router, Profile&)
{
    RedistTransactionXrlOutput<IPv6>* p =
	reinterpret_cast<RedistTransactionXrlOutput<IPv6>*>(this->parent());

    p->set_tid(0);
    p->set_transaction_in_progress(true);
    p->set_transaction_in_error(false);

    XrlRedistTransaction6V0p1Client cl(&xrl_router);
    return cl.send_start_transaction(
	p->xrl_target_name().c_str(),
	callback(this, &StartTransaction<IPv6>::dispatch_complete));
}

template <typename A>
void
StartTransaction<A>::dispatch_complete(const XrlError& xe, const uint32_t* tid)
{
    RedistTransactionXrlOutput<A>* p =
	reinterpret_cast<RedistTransactionXrlOutput<A>*>(this->parent());
    if (xe == XrlError::OKAY()) {
	p->set_tid(*tid);
	this->signal_complete_ok();
	return;
    } else if (xe == XrlError::COMMAND_FAILED()) {
	XLOG_ERROR("Failed to start transaction: %s", xe.str().c_str());
	p->set_transaction_in_progress(false);
	p->set_transaction_in_error(true);
	this->signal_complete_ok();
	return;
    }
    // For now all errors are signalled fatal
    XLOG_ERROR("Fatal error during start transaction: %s",
	       xe.str().c_str());

    this->signal_fatal_failure();
}


// ----------------------------------------------------------------------------
// CommitTransaction implementation

template <>
bool
CommitTransaction<IPv4>::dispatch(XrlRouter& xrl_router, Profile&)
{
    RedistTransactionXrlOutput<IPv4>* p =
	reinterpret_cast<RedistTransactionXrlOutput<IPv4>*>(this->parent());

    uint32_t tid = p->tid();

    p->set_tid(0);	// XXX: reset the tid
    p->set_transaction_in_progress(false);
    p->set_transaction_in_error(false);

    XrlRedistTransaction4V0p1Client cl(&xrl_router);
    return cl.send_commit_transaction(
	p->xrl_target_name().c_str(),
	tid,
	callback(this, &CommitTransaction<IPv4>::dispatch_complete));
}

template <>
bool
CommitTransaction<IPv6>::dispatch(XrlRouter& xrl_router, Profile&)
{
    RedistTransactionXrlOutput<IPv6>* p =
	reinterpret_cast<RedistTransactionXrlOutput<IPv6>*>(this->parent());

    uint32_t tid = p->tid();

    p->set_tid(0);	// XXX: reset the tid
    p->set_transaction_in_progress(false);
    p->set_transaction_in_error(false);

    XrlRedistTransaction6V0p1Client cl(&xrl_router);
    return cl.send_commit_transaction(
	p->xrl_target_name().c_str(),
	tid,
	callback(this, &CommitTransaction<IPv6>::dispatch_complete));
}

template <typename A>
void
CommitTransaction<A>::dispatch_complete(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	this->signal_complete_ok();
	return;
    } else if (xe == XrlError::COMMAND_FAILED()) {
	XLOG_ERROR("Failed to commit transaction: %s", xe.str().c_str());
	this->signal_complete_ok();
	return;
    }
    // For now all errors are signalled fatal
    XLOG_ERROR("Fatal error during commit transaction: %s",
	       xe.str().c_str());

    this->signal_fatal_failure();
}


// ----------------------------------------------------------------------------
// AbortTransaction implementation

template <>
bool
AbortTransaction<IPv4>::dispatch(XrlRouter& xrl_router, Profile&)
{
    RedistTransactionXrlOutput<IPv4>* p =
	reinterpret_cast<RedistTransactionXrlOutput<IPv4>*>(this->parent());

    uint32_t tid = p->tid();

    p->set_tid(0);	// XXX: reset the tid
    p->set_transaction_in_progress(false);
    p->set_transaction_in_error(false);

    XrlRedistTransaction4V0p1Client cl(&xrl_router);
    return cl.send_abort_transaction(
	p->xrl_target_name().c_str(),
	tid,
	callback(this, &AbortTransaction<IPv4>::dispatch_complete));
}

template <>
bool
AbortTransaction<IPv6>::dispatch(XrlRouter& xrl_router, Profile&)
{
    RedistTransactionXrlOutput<IPv6>* p =
	reinterpret_cast<RedistTransactionXrlOutput<IPv6>*>(this->parent());

    uint32_t tid = p->tid();

    p->set_tid(0);	// XXX: reset the tid
    p->set_transaction_in_progress(false);
    p->set_transaction_in_error(false);

    XrlRedistTransaction6V0p1Client cl(&xrl_router);
    return cl.send_abort_transaction(
	p->xrl_target_name().c_str(),
	tid,
	callback(this, &AbortTransaction<IPv6>::dispatch_complete));
}

template <typename A>
void
AbortTransaction<A>::dispatch_complete(const XrlError& xe)
{
    if (xe == XrlError::OKAY()) {
	this->signal_complete_ok();
	return;
    } else if (xe == XrlError::COMMAND_FAILED()) {
	XLOG_ERROR("Failed to abort transaction: %s", xe.str().c_str());
	this->signal_complete_ok();
	return;
    }
    // For now all errors are signalled fatal
    XLOG_ERROR("Fatal error during abort transaction: %s",
	       xe.str().c_str());

    this->signal_fatal_failure();
}


// ----------------------------------------------------------------------------
// RedistTransactionXrlOutput implementation

template <typename A>
RedistTransactionXrlOutput<A>::RedistTransactionXrlOutput(
				Redistributor<A>*	redistributor,
				XrlRouter&		xrl_router,
				Profile&		profile,
				const string&		from_protocol,
				const string&		xrl_target_name,
				const IPNet<A>&		network_prefix,
				const string&		cookie
				)
    : RedistXrlOutput<A>(redistributor, xrl_router, profile, from_protocol,
			 xrl_target_name, network_prefix, cookie),
      _tid(0),
      _transaction_in_progress(false),
      _transaction_in_error(false),
      _transaction_size(0)
{
}

template <typename A>
void
RedistTransactionXrlOutput<A>::add_route(const IPRouteEntry<A>& ipr)
{
    if (this->_profile.enabled(profile_route_rpc_in))
	this->_profile.log(profile_route_rpc_in,
			   c_format("add %s %s %s %u",
				    ipr.protocol().name().c_str(),
				    ipr.net().str().c_str(),
				    ipr.nexthop()->str().c_str(),
				    XORP_UINT_CAST(ipr.metric())));

    bool no_running_tasks = (this->task_count() == 0);

    if (this->transaction_size() == 0)
	this->enqueue_task(new StartTransaction<A>(this));

    //
    // If the accumulated transaction size is too large, commit the
    // current transaction and start a new one.
    //
    if (this->transaction_size() >= MAX_TRANSACTION_SIZE) {
	enqueue_task(new CommitTransaction<A>(this));
	enqueue_task(new StartTransaction<A>(this));
    }

    enqueue_task(new AddTransactionRoute<A>(this, ipr));
    if (no_running_tasks)
	start_running_tasks();
}

template <typename A>
void
RedistTransactionXrlOutput<A>::delete_route(const IPRouteEntry<A>& ipr)
{
    if (this->_profile.enabled(profile_route_rpc_in))
	this->_profile.log(profile_route_rpc_in,
			   c_format("add %s %s",
				    ipr.protocol().name().c_str(),
				    ipr.net().str().c_str()));

    bool no_running_tasks = (this->task_count() == 0);

    if (this->transaction_size() == 0)
	enqueue_task(new StartTransaction<A>(this));

    //
    // If the accumulated transaction size is too large, commit the
    // current transaction and start a new one.
    //
    if (this->transaction_size() >= MAX_TRANSACTION_SIZE) {
	enqueue_task(new CommitTransaction<A>(this));
	enqueue_task(new StartTransaction<A>(this));
    }

    enqueue_task(new DeleteTransactionRoute<A>(this, ipr));
    if (no_running_tasks)
	start_running_tasks();
}

template <typename A>
void
RedistTransactionXrlOutput<A>::starting_route_dump()
{
}

template <typename A>
void
RedistTransactionXrlOutput<A>::finishing_route_dump()
{
}

template <typename A>
void
RedistTransactionXrlOutput<A>::start_running_tasks()
{
    // At start we expect a start transaction followed by an add or delete
    XLOG_ASSERT(this->task_count() == 2);
    this->start_next_task();
}

template <typename A>
void
RedistTransactionXrlOutput<A>::task_completed(Task* task)
{
    dequeue_task(task);
    if (this->task_count() != 0) {
	this->start_next_task();
	return;
    }

    if (transaction_in_progress()) {
	//
	// If transaction in progress, and this is the last add/delete,
	// then send "commit transaction".
	//
	enqueue_task(new CommitTransaction<A>(this));
	this->start_next_task();
	return;
    }
}


// ----------------------------------------------------------------------------
// Instantiations

template class RedistXrlOutput<IPv4>;
template class RedistXrlOutput<IPv6>;

template class RedistTransactionXrlOutput<IPv4>;
template class RedistTransactionXrlOutput<IPv6>;
