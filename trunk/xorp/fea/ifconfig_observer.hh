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

// $XORP: xorp/fea/ifconfig_observer.hh,v 1.19 2007/04/25 07:57:48 pavlin Exp $

#ifndef __FEA_IFCONFIG_OBSERVER_HH__
#define __FEA_IFCONFIG_OBSERVER_HH__

#include "netlink_socket.hh"
#include "routing_socket.hh"


class IfConfig;
class IfTree;

class IfConfigObserver {
public:
    IfConfigObserver(IfConfig& ifconfig);
    
    virtual ~IfConfigObserver();
    
    IfConfig&	ifconfig() { return _ifconfig; }
    
    virtual void register_ifconfig_primary();
    virtual void register_ifconfig_secondary();
    virtual void set_primary() { _is_primary = true; }
    virtual void set_secondary() { _is_primary = false; }
    virtual bool is_primary() const { return _is_primary; }
    virtual bool is_secondary() const { return !_is_primary; }
    virtual bool is_running() const { return _is_running; }

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg) = 0;
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg) = 0;
    
    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer) = 0;

protected:
    // Misc other state
    bool	_is_running;

private:
    IfConfig&	_ifconfig;
    bool	_is_primary;	// True -> primary, false -> secondary method
};

class IfConfigObserverDummy : public IfConfigObserver {
public:
    IfConfigObserverDummy(IfConfig& ifconfig);
    virtual ~IfConfigObserverDummy();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer);
    
private:
    
};

class IfConfigObserverRoutingSocket : public IfConfigObserver,
				      public RoutingSocket,
				      public RoutingSocketObserver {
public:
    IfConfigObserverRoutingSocket(IfConfig& ifconfig);
    virtual ~IfConfigObserverRoutingSocket();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer);
    
    void rtsock_data(const vector<uint8_t>& buffer);
    
private:
    
};

class IfConfigObserverNetlinkSocket : public IfConfigObserver,
				      public NetlinkSocket,
				      public NetlinkSocketObserver {
public:
    IfConfigObserverNetlinkSocket(IfConfig& ifconfig);
    virtual ~IfConfigObserverNetlinkSocket();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer);
    
    void nlsock_data(const vector<uint8_t>& buffer);
    
private:
    
};

class IfConfigObserverIPHelper : public IfConfigObserver {
public:
    IfConfigObserverIPHelper(IfConfig& ifconfig);
    virtual ~IfConfigObserverIPHelper();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer);
};

#endif // __FEA_IFCONFIG_OBSERVER_HH__
