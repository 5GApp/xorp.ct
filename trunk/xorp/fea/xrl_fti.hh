// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_fti.hh,v 1.16 2007/02/16 22:45:52 pavlin Exp $

#ifndef __FEA_XRL_FTI_HH__
#define __FEA_XRL_FTI_HH__

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/fea_fib_client_xif.hh"

#include "fti_transaction.hh"
#include "fte.hh"


/**
 * Helper class for helping with Fti transactions via an Xrl
 * interface.
 *
 * The class provides error messages suitable for Xrl return values
 * and does some extra checking not in the FtiTransactionManager
 * class.
 */
class XrlFtiTransactionManager : public FibTableObserverBase {
public:
    /**
     * Constructor
     *
     * @param e the EventLoop.
     * @param fibconfig the ForwardingTableInterface configuration object.
     * @param max_ops the maximum number of operations pending.
     */
    XrlFtiTransactionManager(EventLoop&	e,
			     FibConfig&	fibconfig,
			     XrlRouter* xrl_router,
			     uint32_t	max_ops = 200)
	: _ftm(e, fibconfig),
	  _max_ops(max_ops),
	  _xrl_fea_fib_client(xrl_router) {
	fibconfig.add_fib_table_observer(this);
    }

    ~XrlFtiTransactionManager() {
	fibconfig().delete_fib_table_observer(this);
    }

    /**
     * Start a Fti transaction.
     * 
     * @param tid the return-by-reference transaction ID.
     * @return the XRL command error.
     */
    XrlCmdError start_transaction(uint32_t& tid);

    /**
     * Commit a Fti transaction.
     * 
     * @param tid the ID of the transaction to commit.
     * @return the XRL command error.
     */
    XrlCmdError commit_transaction(uint32_t tid);

    /**
     * Abort a Fti transaction.
     * 
     * @param tid the ID of the transaction to abort.
     * @return the XRL command error.
     */
    XrlCmdError abort_transaction(uint32_t tid);

    /**
     * Add a Fti operation.
     * 
     * @param tid the transaction ID.
     * @param op the operation to add.
     * @return the XRL command error.
     */
    XrlCmdError add(uint32_t tid, const FtiTransactionManager::Operation& op);

    /**
     * Get the FibConfig entry.
     * 
     * @return a reference to the corresponding FibConfig entry.
     * @see FibConfig.
     */
    FibConfig& fibconfig() { return _ftm.fibconfig(); }

    /**
     * Process a list of IPv4 FIB route changes.
     * 
     * The FIB route changes come from the underlying system.
     * 
     * @param fte_list the list of Fte entries to add or delete.
     */
    void process_fib_changes(const list<Fte4>& fte_list);

    /**
     * Process a list of IPv6 FIB route changes.
     * 
     * The FIB route changes come from the underlying system.
     * 
     * @param fte_list the list of Fte entries to add or delete.
     */
    void process_fib_changes(const list<Fte6>& fte_list);

    /**
     * Add an IPv4 FIB client.
     *
     * @param client_target_name the target name of the client to add.
     * @param send_updates whether updates should be sent.
     * @param send_resolves whether resolve requests should be sent.
     * @return the XRL command error.
     */
    XrlCmdError add_fib_client4(const string& client_target_name,
				const bool send_updates,
				const bool send_resolves);

    /**
     * Add an IPv6 FIB client.
     * 
     * @param client_target_name the target name of the client to add.
     * @param send_updates whether updates should be sent.
     * @param send_resolves whether resolve requests should be sent.
     * @return the XRL command error.
     */
    XrlCmdError add_fib_client6(const string& client_target_name,
				const bool send_updates,
				const bool send_resolves);

    /**
     * Delete an IPv4 FIB client.
     * 
     * @param client_target_name the target name of the client to delete.
     * @return the XRL command error.
     */
    XrlCmdError delete_fib_client4(const string& client_target_name);

    /**
     * Delete an IPv6 FIB client.
     * 
     * @param client_target_name the target name of the client to delete.
     * @return the XRL command error.
     */
    XrlCmdError delete_fib_client6(const string& client_target_name);

    /**
     * Send an XRL to a FIB client to add an IPv4 route.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the route information to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte4.
     */
    int send_fib_client_add_route(const string& target_name,
				  const Fte4& fte);

    /**
     * Send an XRL to a FIB client to add an IPv6 route.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the route information to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte6.
     */
    int send_fib_client_add_route(const string& target_name,
				  const Fte6& fte);

    /**
     * Send an XRL to a FIB client to delete an IPv4 route.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the route information to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte4.
     */
    int send_fib_client_delete_route(const string& target_name,
				     const Fte4& fte);

    /**
     * Send an XRL to a FIB client to delete an IPv6 route.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the route information to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte6.
     */
    int send_fib_client_delete_route(const string& target_name,
				     const Fte6& fte);

    /**
     * Send an XRL to a FIB client to inform it of an IPv4 route miss.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the destination to resolve.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte4.
     */
    int send_fib_client_resolve_route(const string& target_name,
				     const Fte4& fte);

    /**
     * Send an XRL to a FIB client to inform it of an IPv6 route miss.
     *
     * @param target_name the target name of the FIB client.
     * @param fte the Fte with the destination to resolve.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see Fte6.
     */
    int send_fib_client_resolve_route(const string& target_name,
				     const Fte6& fte);

protected:
    FtiTransactionManager  _ftm;
    uint32_t		   _max_ops;	// Maximum operations in a transaction

private:

    void send_fib_client_add_route4_cb(const XrlError& xrl_error,
				       string target_name);
    void send_fib_client_add_route6_cb(const XrlError& xrl_error,
				       string target_name);
    void send_fib_client_delete_route4_cb(const XrlError& xrl_error,
					  string target_name);
    void send_fib_client_delete_route6_cb(const XrlError& xrl_error,
					  string target_name);
    void send_fib_client_resolve_route4_cb(const XrlError& xrl_error,
					  string target_name);
    void send_fib_client_resolve_route6_cb(const XrlError& xrl_error,
					  string target_name);

    /**
     * A template class for storing FIB client information.
     */
    template<class F>
    class FibClient {
    public:
	FibClient(const string& target_name, XrlFtiTransactionManager& xftm)
	    : _target_name(target_name), _xftm(xftm),
	      _send_updates(false), _send_resolves(false) {}

	void	activate(const list<F>& fte_list);
	void	send_fib_client_route_change_cb(const XrlError& xrl_error);

	bool get_send_updates() const { return _send_updates; }
	bool get_send_resolves() const { return _send_resolves; }
	void set_send_updates(const bool sendit) { _send_updates = sendit; }
	void set_send_resolves(const bool sendit) { _send_resolves = sendit; }

    private:
	EventLoop& eventloop() { return _xftm.fibconfig().eventloop(); }
	void	send_fib_client_route_change();

	list<F>			_inform_fib_client_queue;
	XorpTimer		_inform_fib_client_queue_timer;

	string			_target_name;	// Target name of the client
	XrlFtiTransactionManager& _xftm;

	bool			_send_updates;	// Event filters
	bool			_send_resolves;
    };

    typedef FibClient<Fte4>	FibClient4;
    typedef FibClient<Fte6>	FibClient6;

    map<string, FibClient4>	_fib_clients4;
    map<string, FibClient6>	_fib_clients6;

    XrlFeaFibClientV0p1Client	_xrl_fea_fib_client;
};

#endif // __FEA_XRL_FTI_HH__
