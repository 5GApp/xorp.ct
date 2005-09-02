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

#ident "$XORP: xorp/fea/netlink_socket.cc,v 1.32 2005/08/18 15:45:50 bms Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include <algorithm>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#include <errno.h>

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "libcomm/comm_api.h"

#include "netlink_socket.hh"


uint16_t NetlinkSocket::_instance_cnt = 0;
pid_t NetlinkSocket::_pid = getpid();

//
// Netlink Sockets (see netlink(7)) communication with the kernel
//

NetlinkSocket::NetlinkSocket(EventLoop& eventloop)
    : _eventloop(eventloop),
      _fd(-1),
      _seqno(0),
      _instance_no(_instance_cnt++),
      _nl_groups(0),		// XXX: no netlink multicast groups
      _is_multipart_message_read(false)
{
    
}

NetlinkSocket::~NetlinkSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the netlink socket: %s", error_msg.c_str());
    }

    XLOG_ASSERT(_ol.empty());
}

#ifndef HAVE_NETLINK_SOCKETS

int
NetlinkSocket::start(int af, string& error_msg)
{
    error_msg = c_format("The system does not support netlink sockets");
    XLOG_UNREACHABLE();
    return (XORP_ERROR);

    UNUSED(af);
}

int
NetlinkSocket::stop(string& error_msg)
{
    //
    // XXX: Even if the system doesn not support netlink sockets, we
    // still allow to call the no-op stop() method and return success.
    // This is needed to cover the case when a NetlinkSocket is allocated
    // but not used.
    //
    return (XORP_OK);
    UNUSED(error_msg);
}

int
NetlinkSocket::force_read(string& error_msg)
{
    XLOG_UNREACHABLE();

    error_msg = "method not supported";

    return (XORP_ERROR);
}

#else // HAVE_NETLINK_SOCKETS

int
NetlinkSocket::start(int af, string& error_msg)
{
    int			socket_protocol = -1;
    struct sockaddr_nl	snl;
    socklen_t		snl_len;

    if (_fd >= 0)
	return (XORP_OK);
    
    //
    // Select the protocol
    //
    switch (af) {
    case AF_INET:
	socket_protocol = NETLINK_ROUTE;
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	//
	// XXX: the netlink(7) manual page is incorrect that for IPv6
	// we need NETLINK_ROUTE6.
	// The truth is that it has to be NETLINK_ROUTE.
	//
	// socket_protocol = NETLINK_ROUTE6;
	socket_protocol = NETLINK_ROUTE;
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	break;
    }
    
    //
    // Open the socket
    //
    _fd = socket(AF_NETLINK, SOCK_RAW, socket_protocol);
    if (_fd < 0) {
	error_msg = c_format("Could not open netlink socket: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    //
    // Increase the receiving buffer size of the socket to avoid
    // loss of data from the kernel.
    //
    comm_sock_set_rcvbuf(_fd, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN);
    
    // TODO: do we want to make the socket non-blocking?
    
    //
    // Bind the socket
    //
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid    = 0;		// nl_pid = 0 if destination is the kernel
    snl.nl_groups = _nl_groups;
    if (bind(_fd, reinterpret_cast<struct sockaddr*>(&snl), sizeof(snl)) < 0) {
	error_msg = c_format("bind(AF_NETLINK) failed: %s", strerror(errno));
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
    
    //
    // Double-check the result socket is AF_NETLINK
    //
    snl_len = sizeof(snl);
    if (getsockname(_fd, reinterpret_cast<struct sockaddr*>(&snl), &snl_len) < 0) {
	error_msg = c_format("getsockname(AF_NETLINK) failed: %s",
			     strerror(errno));
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
    if (snl_len != sizeof(snl)) {
	error_msg = c_format("Wrong address length of AF_NETLINK socket: "
			     "%u instead of %u",
			     XORP_UINT_CAST(snl_len),
			     XORP_UINT_CAST(sizeof(snl)));
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
    if (snl.nl_family != AF_NETLINK) {
	error_msg = c_format("Wrong address family of AF_NETLINK socket: "
			     "%d instead of %d",
			     snl.nl_family, AF_NETLINK);
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
#if 0				// XXX
    // XXX: 'nl_pid' is supposed to be defined as 'pid_t'
    if ( (pid_t)snl.nl_pid != pid()) {
	error_msg = c_format("Wrong nl_pid of AF_NETLINK socket: "
			     "%u instead of %u",
			     XORP_UINT_CAST(snl.nl_pid),
			     XORP_UINT_CAST(pid()));
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
#endif // 0
    
    //
    // Add the socket to the event loop
    //
    if (_eventloop.add_ioevent_cb(_fd, IOT_READ,
				callback(this, &NetlinkSocket::io_event))
	== false) {
	error_msg = c_format("Failed to add netlink socket to EventLoop");
	close(_fd);
	_fd = -1;
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
NetlinkSocket::stop(string& error_msg)
{
    if (_fd >= 0) {
	_eventloop.remove_ioevent_cb(_fd);
	close(_fd);
	_fd = -1;
    }
    
    return (XORP_OK);

    UNUSED(error_msg);
}

ssize_t
NetlinkSocket::write(const void* data, size_t nbytes)
{
    _seqno++;
    return ::write(_fd, data, nbytes);
}

ssize_t
NetlinkSocket::sendto(const void* data, size_t nbytes, int flags,
		      const struct sockaddr* to, socklen_t tolen)
{
    _seqno++;
    return ::sendto(_fd, data, nbytes, flags, to, tolen);
}

int
NetlinkSocket::force_read(string& error_msg)
{
    vector<uint8_t> message;
    vector<uint8_t> buffer(NLSOCK_BYTES);
    size_t off = 0;
    size_t last_mh_off = 0;
    
    for ( ; ; ) {
	ssize_t got;
	// Find how much data is queued in the first message
	do {
	    got = recv(_fd, &buffer[0], buffer.size(),
		       MSG_DONTWAIT | MSG_PEEK);
	    if ((got < 0) && (errno == EINTR))
		continue;	// XXX: the receive was interrupted by a signal
	    if ((got < 0) || (got < (ssize_t)buffer.size()))
		break;		// The buffer is big enough
	    buffer.resize(buffer.size() + NLSOCK_BYTES);
	} while (true);
	
	got = read(_fd, &buffer[0], buffer.size());
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    error_msg = c_format("Netlink socket read error: %s",
			      strerror(errno));
	    return (XORP_ERROR);
	}
	message.resize(message.size() + got);
	memcpy(&message[off], &buffer[0], got);
	off += got;
	
	if ((off - last_mh_off) < (ssize_t)sizeof(struct nlmsghdr)) {
	    error_msg = c_format("Netlink socket read failed: "
				 "message truncated: "
				 "received %d bytes instead of (at least) %u "
				 "bytes",
				 XORP_INT_CAST(got),
				 XORP_UINT_CAST(sizeof(struct nlmsghdr)));
	    return (XORP_ERROR);
	}
	
	//
	// If this is a multipart message, it must be terminated by NLMSG_DONE
	//
	bool is_end_of_message = true;
	size_t new_size = off - last_mh_off;
	const struct nlmsghdr* mh;
	for (mh = reinterpret_cast<const struct nlmsghdr*>(&message[last_mh_off]);
	     NLMSG_OK(mh, new_size);
	     mh = NLMSG_NEXT(const_cast<struct nlmsghdr*>(mh), new_size)) {
	    XLOG_ASSERT(mh->nlmsg_len <= buffer.size());
	    if ((mh->nlmsg_flags & NLM_F_MULTI)
		|| _is_multipart_message_read) {
		is_end_of_message = false;
		if (mh->nlmsg_type == NLMSG_DONE)
		    is_end_of_message = true;
	    }
	}
	last_mh_off = reinterpret_cast<size_t>(mh) - reinterpret_cast<size_t>(&message[0]);
	if (is_end_of_message)
	    break;
    }
    XLOG_ASSERT(last_mh_off == message.size());
    
    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->nlsock_data(&message[0], message.size());
    }

    return (XORP_OK);
}

int
NetlinkSocket::force_recvfrom(int flags, struct sockaddr* from,
			      socklen_t* fromlen, string& error_msg)
{
    vector<uint8_t> message;
    vector<uint8_t> buffer(NLSOCK_BYTES);
    size_t off = 0;
    size_t last_mh_off = 0;
    
    for ( ; ; ) {
	ssize_t got;
	// Find how much data is queued in the first message
	do {
	    got = recv(_fd, &buffer[0], buffer.size(),
		       MSG_DONTWAIT | MSG_PEEK);
	    if ((got < 0) && (errno == EINTR))
		continue;	// XXX: the receive was interrupted by a signal
	    if ((got < 0) || (got < (ssize_t)buffer.size()))
		break;		// The buffer is big enough
	    buffer.resize(buffer.size() + NLSOCK_BYTES);
	} while (true);
	
	got = recvfrom(_fd, &buffer[0], buffer.size(), flags, from, fromlen);
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    error_msg = c_format("Netlink socket recvfrom error: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}
	message.resize(message.size() + got);
	memcpy(&message[off], &buffer[0], got);
	off += got;
	
	if ((off - last_mh_off) < (ssize_t)sizeof(struct nlmsghdr)) {
	    error_msg = c_format("Netlink socket recvfrom failed: "
				 "message truncated: "
				 "received %d bytes instead of (at least) %u "
				 "bytes",
				 XORP_INT_CAST(got),
				 XORP_UINT_CAST(sizeof(struct nlmsghdr)));
	    return (XORP_ERROR);
	}
	
	//
	// If this is a multipart message, it must be terminated by NLMSG_DONE
	//
	bool is_end_of_message = true;
	size_t new_size = off - last_mh_off;
	const struct nlmsghdr* mh;
	for (mh = reinterpret_cast<const struct nlmsghdr*>(&message[last_mh_off]);
	     NLMSG_OK(mh, new_size);
	     mh = NLMSG_NEXT(const_cast<struct nlmsghdr*>(mh), new_size)) {
	    XLOG_ASSERT(mh->nlmsg_len <= buffer.size());
	    if ((mh->nlmsg_flags & NLM_F_MULTI)
		|| _is_multipart_message_read) {
		is_end_of_message = false;
		if (mh->nlmsg_type == NLMSG_DONE)
		    is_end_of_message = true;
	    }
	}
	last_mh_off = reinterpret_cast<size_t>(mh) - reinterpret_cast<size_t>(&message[0]);
	if (is_end_of_message)
	    break;
    }
    XLOG_ASSERT(last_mh_off == message.size());
    
    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->nlsock_data(&message[0], message.size());
    }

    return (XORP_OK);
}

int
NetlinkSocket::force_recvmsg(int flags, string& error_msg)
{
    vector<uint8_t> message;
    vector<uint8_t> buffer(NLSOCK_BYTES);
    size_t off = 0;
    size_t last_mh_off = 0;
    struct iovec	iov;
    struct msghdr	msg;
    struct sockaddr_nl	snl;
    
    // Set the socket
    memset(&snl, 0, sizeof(snl));
    snl.nl_family = AF_NETLINK;
    
    // Init the recvmsg() arguments
    iov.iov_base = &buffer[0];
    iov.iov_len = buffer.size();
    msg.msg_name = &snl;
    msg.msg_namelen = sizeof(snl);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    
    for ( ; ; ) {
	ssize_t got;
	// Find how much data is queued in the first message
	do {
	    got = recv(_fd, &buffer[0], buffer.size(),
		       MSG_DONTWAIT | MSG_PEEK);
	    if ((got < 0) && (errno == EINTR))
		continue;	// XXX: the receive was interrupted by a signal
	    if ((got < 0) || (got < (ssize_t)buffer.size()))
		break;		// The buffer is big enough
	    buffer.resize(buffer.size() + NLSOCK_BYTES);
	} while (true);
	
	// Re-init the iov argument
	iov.iov_base = &buffer[0];
	iov.iov_len = buffer.size();
	
	got = recvmsg(_fd, &msg, flags);
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    error_msg = c_format("Netlink socket recvmsg error: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}
	if (msg.msg_namelen != sizeof(snl)) {
	    error_msg = c_format("Netlink socket recvmsg error: "
				 "sender address length %d instead of %u",
				 XORP_INT_CAST(msg.msg_namelen),
				 XORP_UINT_CAST(sizeof(snl)));
	    return (XORP_ERROR);
	}
	message.resize(message.size() + got);
	memcpy(&message[off], &buffer[0], got);
	off += got;
	
	if ((off - last_mh_off) < (ssize_t)sizeof(struct nlmsghdr)) {
	    error_msg = c_format("Netlink socket recvmsg failed: "
				 "message truncated: "
				 "received %d bytes instead of (at least) %u "
				 "bytes",
				 XORP_INT_CAST(got),
				 XORP_UINT_CAST(sizeof(struct nlmsghdr)));
	    return (XORP_ERROR);
	}
	
	//
	// If this is a multipart message, it must be terminated by NLMSG_DONE
	//
	bool is_end_of_message = true;
	size_t new_size = off - last_mh_off;
	const struct nlmsghdr* mh;
	for (mh = reinterpret_cast<const struct nlmsghdr*>(&message[last_mh_off]);
	     NLMSG_OK(mh, new_size);
	     mh = NLMSG_NEXT(const_cast<struct nlmsghdr*>(mh), new_size)) {
	    XLOG_ASSERT(mh->nlmsg_len <= buffer.size());
	    if ((mh->nlmsg_flags & NLM_F_MULTI)
		|| _is_multipart_message_read) {
		is_end_of_message = false;
		if (mh->nlmsg_type == NLMSG_DONE)
		    is_end_of_message = true;
	    }
	}
	last_mh_off = reinterpret_cast<size_t>(mh) - reinterpret_cast<size_t>(&message[0]);
	if (is_end_of_message)
	    break;
    }
    XLOG_ASSERT(last_mh_off == message.size());
    
    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->nlsock_data(&message[0], message.size());
    }

    return (XORP_OK);
}

void
NetlinkSocket::io_event(XorpFd fd, IoEventType type)
{
    string error_msg;

    XLOG_ASSERT(fd == _fd);
    XLOG_ASSERT(type == IOT_READ);
    if (force_read(error_msg) != XORP_OK) {
	XLOG_ERROR("Error force_read() from netlink socket: %s",
		   error_msg.c_str());
    }
}

#endif // HAVE_NETLINK_SOCKETS

//
// Observe netlink sockets activity
//

struct NetlinkSocketPlumber {
    typedef NetlinkSocket::ObserverList ObserverList;

    static void
    plumb(NetlinkSocket& r, NetlinkSocketObserver* o)
    {
	ObserverList& ol = r._ol;
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	debug_msg("Plumbing NetlinkSocketObserver %p to NetlinkSocket%p\n",
		  o, &r);
	XLOG_ASSERT(i == ol.end());
	ol.push_back(o);
    }
    static void
    unplumb(NetlinkSocket& r, NetlinkSocketObserver* o)
    {
	ObserverList& ol = r._ol;
	debug_msg("Unplumbing NetlinkSocketObserver %p from "
		  "NetlinkSocket %p\n", o, &r);
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	XLOG_ASSERT(i != ol.end());
	ol.erase(i);
    }
};

NetlinkSocketObserver::NetlinkSocketObserver(NetlinkSocket4& ns4,
					     NetlinkSocket6& ns6)
    : _ns4(ns4),
      _ns6(ns6)
{
    NetlinkSocketPlumber::plumb(ns4, this);
    NetlinkSocketPlumber::plumb(ns6, this);
}

NetlinkSocketObserver::~NetlinkSocketObserver()
{
    NetlinkSocketPlumber::unplumb(_ns4, this);
    NetlinkSocketPlumber::unplumb(_ns6, this);
}

NetlinkSocket&
NetlinkSocketObserver::netlink_socket4()
{
    return _ns4;
}

NetlinkSocket&
NetlinkSocketObserver::netlink_socket6()
{
    return _ns6;
}

NetlinkSocketReader::NetlinkSocketReader(NetlinkSocket4& ns4,
					 NetlinkSocket6& ns6)
    : NetlinkSocketObserver(ns4, ns6),
      _ns4(ns4),
      _ns6(ns6),
      _cache_valid(false),
      _cache_seqno(0)
{

}

NetlinkSocketReader::~NetlinkSocketReader()
{

}

/**
 * Force the reader to receive data from the IPv4 netlink socket.
 *
 * @param seqno the sequence number of the data to receive.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
NetlinkSocketReader::receive_data4(uint32_t seqno, string& error_msg)
{
    _cache_seqno = seqno;
    _cache_valid = false;
    while (_cache_valid == false) {
	if (_ns4.force_read(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Force the reader to receive data from the IPv6 netlink socket.
 *
 * @param seqno the sequence number of the data to receive.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
NetlinkSocketReader::receive_data6(uint32_t seqno, string& error_msg)
{
    _cache_seqno = seqno;
    _cache_valid = false;
    while (_cache_valid == false) {
	if (_ns6.force_read(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Force the reader to receive data from the specified netlink socket.
 *
 * @param ns the netlink socket to receive the data from.
 * @param seqno the sequence number of the data to receive.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
NetlinkSocketReader::receive_data(NetlinkSocket& ns, uint32_t seqno,
				  string& error_msg)
{
    _cache_seqno = seqno;
    _cache_valid = false;
    while (_cache_valid == false) {
	if (ns.force_read(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Receive data from the netlink socket.
 *
 * Note that this method is called asynchronously when the netlink socket
 * has data to receive, therefore it should never be called directly by
 * anything else except the netlink socket facility itself.
 *
 * @param data the buffer with the received data.
 * @param nbytes the number of bytes in the @param data buffer.
 */
void
NetlinkSocketReader::nlsock_data(const uint8_t* data, size_t nbytes)
{
#ifndef HAVE_NETLINK_SOCKETS
    UNUSED(data);
    UNUSED(nbytes);

    XLOG_UNREACHABLE();

#else // HAVE_NETLINK_SOCKETS

    size_t d = 0, off = 0;
    // XXX: we don't use the pid (see below why).
    // pid_t my_pid = _ns4.pid();

    //
    // Copy data that has been requested to be cached by setting _cache_seqno
    //
    _cache_data.resize(nbytes);
    while (d < nbytes) {
	const struct nlmsghdr* nlh = reinterpret_cast<const struct nlmsghdr*>(data + d);
	if (nlh->nlmsg_seq == _cache_seqno) {
	    //
	    // TODO: XXX: here we should add the following check as well:
	    //       ((pid_t)nlh->nlmsg_pid == my_pid)
	    // However, it appears that on return Linux doesn't fill-in
	    // nlh->nlmsg_pid to our pid (e.g., it may be set to 0xffffefff).
	    // Unfortunately, Linux's netlink(7) is not helpful on the
	    // subject, hence we ignore this additional test.
	    //
	    XLOG_ASSERT(nbytes - d >= nlh->nlmsg_len);
	    memcpy(&_cache_data[off], nlh, nlh->nlmsg_len);
	    off += nlh->nlmsg_len;
	    _cache_valid = true;
	}
	d += nlh->nlmsg_len;
    }
#endif // HAVE_NETLINK_SOCKETS
}
