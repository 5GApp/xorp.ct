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

// $XORP: xorp/fea/ifconfig_get.hh,v 1.19 2004/12/03 04:10:47 pavlin Exp $

#ifndef __FEA_IFCONFIG_GET_HH__
#define __FEA_IFCONFIG_GET_HH__


#include "libxorp/xorp.h"
#include "libxorp/ipvx.hh"

#include "click_socket.hh"
#include "netlink_socket.hh"

class IfConfig;
class IfTree;
struct ifaddrs;

class IfConfigGet {
public:
    IfConfigGet(IfConfig& ifc);
    
    virtual ~IfConfigGet();
    
    IfConfig&	ifc() { return _ifc; }
    
    virtual void register_ifc_primary();
    virtual void register_ifc_secondary();
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
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config) = 0;
    
    /**
     * Parse information about network interface configuration change from
     * the underlying system.
     * 
     * The information to parse is in "struct ifaddrs" format
     * (e.g., obtained by getifaddrs(3) mechanism).
     * 
     * @param it the IfTree storage to store the parsed information.
     * @param ifap a linked list of the network interfaces on the
     * local machine.
     * @return true on success, otherwise false.
     * @see IfTree.
     */
    bool parse_buffer_ifaddrs(IfTree& it, const ifaddrs **ifap);

    /**
     * Parse information about network interface configuration change from
     * the underlying system.
     * 
     * The information to parse is in RTM format
     * (e.g., obtained by routing sockets or by sysctl(3) mechanism).
     * 
     * @param it the IfTree storage to store the parsed information.
     * @param buf the buffer with the data to parse.
     * @param buf_bytes the size of the data in the buffer.
     * @return true on success, otherwise false.
     * @see IfTree.
     */
    bool parse_buffer_rtm(IfTree& it, const uint8_t *buf, size_t buf_bytes);

    /**
     * Parse information about network interface configuration change from
     * the underlying system.
     * 
     * The information to parse is in "struct ifreq" format
     * (e.g., obtained by ioctl(SIOCGIFCONF) mechanism).
     * 
     * @param it the IfTree storage to store the parsed information.
     * @param family the address family to consider only (e.g., AF_INET
     * or AF_INET6 for IPv4 and IPv6 respectively).
     * @param buf the buffer with the data to parse.
     * @param buf_bytes the size of the data in the buffer.
     * @return true on success, otherwise false.
     * @see IfTree.
     */
    bool parse_buffer_ifreq(IfTree& it, int family, const uint8_t *buf,
			    size_t buf_bytes);

    /**
     * Parse information about network interface configuration change from
     * the underlying system.
     * 
     * The information to parse is in NETLINK format
     * (e.g., obtained by netlink(7) sockets mechanism).
     * 
     * @param it the IfTree storage to store the parsed information.
     * @param buf the buffer with the data to parse.
     * @param buf_bytes the size of the data in the buffer.
     * @return true on success, otherwise false.
     * @see IfTree.
     */
    bool parse_buffer_nlm(IfTree& it, const uint8_t *buf, size_t buf_bytes);
    
protected:
    static string iff_flags(uint32_t flags);
    int sock(int family);

    int	_s4;
    int _s6;

    // Misc other state
    bool	_is_running;
    
private:
    IfConfig&	_ifc;
    bool	_is_primary;	// True -> primary, false -> secondary method
};

class IfConfigGetDummy : public IfConfigGet {
public:
    IfConfigGetDummy(IfConfig& ifc);
    virtual ~IfConfigGetDummy();

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
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);
    
private:
    
};

class IfConfigGetGetifaddrs : public IfConfigGet {
public:
    IfConfigGetGetifaddrs(IfConfig& ifc);
    virtual ~IfConfigGetGetifaddrs();

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
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);

private:
    bool read_config(IfTree& it);
};

class IfConfigGetSysctl : public IfConfigGet {
public:
    IfConfigGetSysctl(IfConfig& ifc);
    virtual ~IfConfigGetSysctl();

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
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);
    
private:
    bool read_config(IfTree& it);
};

class IfConfigGetIoctl : public IfConfigGet {
public:
    IfConfigGetIoctl(IfConfig& ifc);
    virtual ~IfConfigGetIoctl();
    
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
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);
    
private:
    bool read_config(IfTree& it);
};

class IfConfigGetProcLinux : public IfConfigGet {
public:
    IfConfigGetProcLinux(IfConfig& ifc);
    virtual ~IfConfigGetProcLinux();
    
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
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);
    
private:
    bool read_config(IfTree& it);

    static const string PROC_LINUX_NET_DEVICES_FILE_V4;
    static const string PROC_LINUX_NET_DEVICES_FILE_V6;
};

class IfConfigGetNetlink : public IfConfigGet,
			   public NetlinkSocket4,
			   public NetlinkSocket6 {
public:
    IfConfigGetNetlink(IfConfig& ifc);
    virtual ~IfConfigGetNetlink();

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
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);

private:
    bool read_config(IfTree& it);

    NetlinkSocketReader	_ns_reader;
};

class IfConfigGetClick : public IfConfigGet,
			 public ClickSocket {
public:
    IfConfigGetClick(IfConfig& ifc);
    virtual ~IfConfigGetClick();
    
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
     * Pull the network interface information from the underlying system.
     * 
     * @param config the IfTree storage to store the pulled information.
     * @return true on success, otherwise false.
     */
    virtual bool pull_config(IfTree& config);
    
private:
    bool read_config(IfTree& it);

    ClickSocketReader	_cs_reader;
};

#endif // __FEA_IFCONFIG_GET_HH__
