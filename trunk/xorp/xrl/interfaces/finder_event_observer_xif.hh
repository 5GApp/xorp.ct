/*
 * Copyright (c) 2001-2004 International Computer Science Institute
 * See LICENSE file for licensing, conditions, and warranties on use.
 *
 * DO NOT EDIT THIS FILE - IT IS PROGRAMMATICALLY GENERATED
 *
 * Generated by 'clnt-gen'.
 *
 * $XORP: xorp/xrl/interfaces/finder_event_observer_xif.hh,v 1.7 2004/06/10 22:42:01 hodson Exp $
 */

#ifndef __XRL_INTERFACES_FINDER_EVENT_OBSERVER_XIF_HH__
#define __XRL_INTERFACES_FINDER_EVENT_OBSERVER_XIF_HH__

#undef XORP_LIBRARY_NAME
#define XORP_LIBRARY_NAME "XifFinderEventObserver"

#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxipc/xrl.hh"
#include "libxipc/xrl_error.hh"
#include "libxipc/xrl_sender.hh"


class XrlFinderEventObserverV0p1Client {
public:
    XrlFinderEventObserverV0p1Client(XrlSender* s) : _sender(s) {}
    virtual ~XrlFinderEventObserverV0p1Client() {}

    typedef XorpCallback1<void, const XrlError&>::RefPtr XrlTargetBirthCB;
    /**
     *  Send Xrl intended to:
     *
     *  Announce target birth to observer.
     *
     *  @param tgt_name Xrl Target name
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    bool send_xrl_target_birth(
	const char*	target_name,
	const string&	target_class,
	const string&	target_instance,
	const XrlTargetBirthCB&	cb
    );

    typedef XorpCallback1<void, const XrlError&>::RefPtr XrlTargetDeathCB;
    /**
     *  Send Xrl intended to:
     *
     *  Announce target death to observer.
     *
     *  @param tgt_name Xrl Target name
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    bool send_xrl_target_death(
	const char*	target_name,
	const string&	target_class,
	const string&	target_instance,
	const XrlTargetDeathCB&	cb
    );

protected:
    XrlSender* _sender;

private:
    void unmarshall_xrl_target_birth(
	const XrlError&	e,
	XrlArgs*	a,
	XrlTargetBirthCB		cb
    );

    void unmarshall_xrl_target_death(
	const XrlError&	e,
	XrlArgs*	a,
	XrlTargetDeathCB		cb
    );

};

#endif /* __XRL_INTERFACES_FINDER_EVENT_OBSERVER_XIF_HH__ */
