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

#ident "$XORP: xorp/rip/auth.cc,v 1.29 2006/03/25 03:44:13 pavlin Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rip_module.h"

#include <functional>
#include <openssl/md5.h>

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"

#include "constants.hh"
#include "auth.hh"

// ----------------------------------------------------------------------------
// AuthHandlerBase implementation

AuthHandlerBase::~AuthHandlerBase()
{
}

const string&
AuthHandlerBase::error() const
{
    return _err;
}

inline void
AuthHandlerBase::reset_error()
{
    if (_err.empty() == false)
	_err.erase();
}

inline void
AuthHandlerBase::set_error(const string& err)
{
    _err = err;
}

// ----------------------------------------------------------------------------
// NullAuthHandler implementation

const char*
NullAuthHandler::name() const
{
    return auth_type_name();
}

const char*
NullAuthHandler::auth_type_name()
{
    return "none";
}

void
NullAuthHandler::reset()
{
}

uint32_t
NullAuthHandler::head_entries() const
{
    return 0;
}

uint32_t
NullAuthHandler::max_routing_entries() const
{
    return RIPv2_ROUTES_PER_PACKET;
}

bool
NullAuthHandler::authenticate_inbound(const uint8_t*		packet,
				      size_t			packet_bytes,
				      const PacketRouteEntry<IPv4>*& entries,
				      uint32_t&			n_entries,
				      const IPv4&,
				      bool)
{
    entries = 0;
    n_entries = 0;

    if (packet_bytes > RIPv2_MAX_PACKET_BYTES) {
	set_error(c_format("packet too large (%u bytes)",
			   XORP_UINT_CAST(packet_bytes)));
	return false;
    } else if (packet_bytes < RIPv2_MIN_PACKET_BYTES) {
	set_error(c_format("packet too small (%u bytes)",
			   XORP_UINT_CAST(packet_bytes)));
	return false;
    }

    size_t entry_bytes = packet_bytes - sizeof(RipPacketHeader);
    if (entry_bytes % sizeof(PacketRouteEntry<IPv4>)) {
	set_error(c_format("non-integral route entries (%u bytes)",
			    XORP_UINT_CAST(entry_bytes)));
	return false;
    }

    n_entries = entry_bytes / sizeof(PacketRouteEntry<IPv4>);
    if (n_entries == 0) {
	return true;
    }

    entries = reinterpret_cast<const PacketRouteEntry<IPv4>*>
	(packet + sizeof(RipPacketHeader));

    // Reject packet if first entry is authentication data
    if (entries[0].is_auth_entry()) {
	set_error(c_format("unexpected authentication data (type %d)",
			   entries[0].tag()));
	entries = 0;
	n_entries = 0;
	return false;
    }

    reset_error();
    return true;
}

bool
NullAuthHandler::authenticate_outbound(RipPacket<IPv4>&	packet,
				       list<RipPacket<IPv4> *>& auth_packets,
				       size_t&		n_routes)
{
    // XXX: nothing to do so just create a single copy
    RipPacket<IPv4>* copy_packet = new RipPacket<IPv4>(packet);
    auth_packets.push_back(copy_packet);

    reset_error();

    n_routes = packet.data_bytes() - sizeof(RipPacketHeader);
    n_routes /= sizeof(PacketRouteEntry<IPv4>);

    return (true);
}


// ----------------------------------------------------------------------------
// Plaintext handler implementation

const char*
PlaintextAuthHandler::name() const
{
    return auth_type_name();
}

const char*
PlaintextAuthHandler::auth_type_name()
{
    return "simple";
}

void
PlaintextAuthHandler::reset()
{
}

uint32_t
PlaintextAuthHandler::head_entries() const
{
    return 1;
}

uint32_t
PlaintextAuthHandler::max_routing_entries() const
{
    return RIPv2_ROUTES_PER_PACKET - 1;
}

bool
PlaintextAuthHandler::authenticate_inbound(const uint8_t*	packet,
					   size_t		packet_bytes,
					   const PacketRouteEntry<IPv4>*& entries,
					   uint32_t&		n_entries,
					   const IPv4&,
					   bool)
{
    entries = 0;
    n_entries = 0;

    if (packet_bytes > RIPv2_MAX_PACKET_BYTES) {
	set_error(c_format("packet too large (%u bytes)",
			   XORP_UINT_CAST(packet_bytes)));
	return false;
    }

    if (packet_bytes < RIPv2_MIN_AUTH_PACKET_BYTES) {
	set_error(c_format("packet too small (%u bytes)",
			   XORP_UINT_CAST(packet_bytes)));
	return false;
    }

    size_t entry_bytes = packet_bytes - sizeof(RipPacketHeader);
    if (entry_bytes % sizeof(PacketRouteEntry<IPv4>)) {
	set_error(c_format("non-integral route entries (%u bytes)",
			   XORP_UINT_CAST(entry_bytes)));
	return false;
    }

    const PlaintextPacketRouteEntry4* ppr =
	reinterpret_cast<const PlaintextPacketRouteEntry4*>
	(packet + sizeof(RipPacketHeader));

    if (ppr->addr_family() != PlaintextPacketRouteEntry4::ADDR_FAMILY) {
	set_error("not an authenticated packet");
	return false;
    } else if (ppr->auth_type() != PlaintextPacketRouteEntry4::AUTH_TYPE) {
	set_error("not a plaintext authenticated packet");
	return false;
    }

    string passwd = ppr->password();
    if (passwd != key()) {
	set_error(c_format("wrong password \"%s\"", passwd.c_str()));
	return false;
    }

    reset_error();
    n_entries = entry_bytes / sizeof(PacketRouteEntry<IPv4>) - 1;
    if (n_entries)
	entries = reinterpret_cast<const PacketRouteEntry<IPv4>*>(ppr + 1);

    return true;
}

bool
PlaintextAuthHandler::authenticate_outbound(RipPacket<IPv4>&	packet,
					    list<RipPacket<IPv4> *>& auth_packets,
					    size_t&		n_routes)
{
    PacketRouteEntry<IPv4>* first_entry = NULL;

    if (head_entries() > 0)
	first_entry = packet.route_entry(0);

    static_assert(sizeof(*first_entry) == 20);
    static_assert(sizeof(PlaintextPacketRouteEntry4) == 20);
    XLOG_ASSERT(packet.data_ptr() + sizeof(RipPacketHeader) ==
		reinterpret_cast<const uint8_t*>(first_entry));

    PlaintextPacketRouteEntry4* ppr =
	reinterpret_cast<PlaintextPacketRouteEntry4*>(first_entry);
    ppr->initialize(key());

    // XXX: just create a single copy
    RipPacket<IPv4>* copy_packet = new RipPacket<IPv4>(packet);
    auth_packets.push_back(copy_packet);

    reset_error();

    n_routes = packet.data_bytes() - sizeof(RipPacketHeader);
    n_routes /= sizeof(PacketRouteEntry<IPv4>);
    n_routes--;		// XXX: exclude the first (authentication) entry

    return (true);
}

const string&
PlaintextAuthHandler::key() const
{
    return _key;
}

void
PlaintextAuthHandler::set_key(const string& plaintext_key)
{
    _key = string(plaintext_key, 0, 16);
}


// ----------------------------------------------------------------------------
// MD5AuthHandler::MD5Key implementation

MD5AuthHandler::MD5Key::MD5Key(uint8_t		key_id,
			       const string&	key,
			       const TimeVal&	start_timeval,
			       const TimeVal&	end_timeval,
			       XorpTimer	start_timer,
			       XorpTimer	stop_timer)
    : _id(key_id),
      _start_timeval(start_timeval),
      _end_timeval(end_timeval),
      _is_persistent(false),
      _o_seqno(0),
      _start_timer(start_timer),
      _stop_timer(stop_timer)
{
    string::size_type n = key.copy(_key_data, 16);
    if (n < KEY_BYTES) {
	memset(_key_data + n, 0, KEY_BYTES - n);
    }
}

string
MD5AuthHandler::MD5Key::key() const
{
    return string(_key_data, 0, 16);
}

bool
MD5AuthHandler::MD5Key::valid_at(const TimeVal& when) const
{
    if (is_persistent())
	return true;

    return ((_start_timeval <= when) && (when <= _end_timeval));
}

void
MD5AuthHandler::MD5Key::reset()
{
    //
    // Reset the seqno
    //
    _lr_seqno.clear();

    //
    // Reset the flag that a packet has been received
    //
    _pkts_recv.clear();
}

void
MD5AuthHandler::MD5Key::reset(const IPv4& src_addr)
{
    map<IPv4, uint32_t>::iterator seqno_iter;
    map<IPv4, bool>::iterator recv_iter;

    //
    // Reset the seqno
    //
    seqno_iter = _lr_seqno.find(src_addr);
    if (seqno_iter != _lr_seqno.end())
	_lr_seqno.erase(seqno_iter);

    //
    // Reset the flag that a packet has been received
    //
    recv_iter = _pkts_recv.find(src_addr);
    if (recv_iter != _pkts_recv.end())
	_pkts_recv.erase(recv_iter);
}

bool
MD5AuthHandler::MD5Key::packets_received(const IPv4& src_addr) const
{
    map<IPv4, bool>::const_iterator iter;

    iter = _pkts_recv.find(src_addr);
    if (iter == _pkts_recv.end())
	return (false);

    return (iter->second);
}

uint32_t
MD5AuthHandler::MD5Key::last_seqno_recv(const IPv4& src_addr) const
{
    map<IPv4, uint32_t>::const_iterator iter;

    iter = _lr_seqno.find(src_addr);
    if (iter == _lr_seqno.end())
	return (0);

    return (iter->second);
}

void
MD5AuthHandler::MD5Key::set_last_seqno_recv(const IPv4& src_addr,
					    uint32_t seqno)
{
    map<IPv4, uint32_t>::iterator seqno_iter;
    map<IPv4, bool>::iterator recv_iter;

    //
    // Set the seqno
    //
    seqno_iter = _lr_seqno.find(src_addr);
    if (seqno_iter == _lr_seqno.end())
	_lr_seqno.insert(make_pair(src_addr, seqno));
    else
	seqno_iter->second = seqno;

    //
    // Set the flag that a packet has been received
    //
    recv_iter = _pkts_recv.find(src_addr);
    if (recv_iter == _pkts_recv.end())
	_pkts_recv.insert(make_pair(src_addr, true));
    else
	recv_iter->second = true;
}


// ----------------------------------------------------------------------------
// MD5AuthHandler implementation

MD5AuthHandler::MD5AuthHandler(EventLoop& eventloop)
    : _eventloop(eventloop)
{
}

const char*
MD5AuthHandler::name() const
{
    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (_valid_key_chain.empty()) {
	return (_null_handler.name());
    }

    return auth_type_name();
}

const char*
MD5AuthHandler::auth_type_name()
{
    return "md5";
}

void
MD5AuthHandler::reset()
{
    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (_valid_key_chain.empty()) {
	_null_handler.reset();
	return;
    }

    reset_keys();
}

uint32_t
MD5AuthHandler::head_entries() const
{
    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (_valid_key_chain.empty()) {
	return (_null_handler.head_entries());
    }

    return 1;
}

uint32_t
MD5AuthHandler::max_routing_entries() const
{
    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (_valid_key_chain.empty()) {
	return (_null_handler.max_routing_entries());
    }

    return RIPv2_ROUTES_PER_PACKET - 1;
}

bool
MD5AuthHandler::authenticate_inbound(const uint8_t*		packet,
				     size_t			packet_bytes,
				     const PacketRouteEntry<IPv4>*& entries,
				     uint32_t&			n_entries,
				     const IPv4&		src_addr,
				     bool			new_peer)
{
    static_assert(sizeof(MD5PacketTrailer) == 20);

    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (_valid_key_chain.empty()) {
	if (_null_handler.authenticate_inbound(packet, packet_bytes,
					       entries, n_entries,
					       src_addr, new_peer)
	    != true) {
	    set_error(_null_handler.error());
	    return (false);
	}
	reset_error();
	return (true);
    }

    entries = 0;
    n_entries = 0;

    if (packet_bytes > RIPv2_MAX_PACKET_BYTES + sizeof(MD5PacketTrailer)) {
	set_error(c_format("packet too large (%u bytes)",
			   XORP_UINT_CAST(packet_bytes)));
	return false;
    }

    if (packet_bytes < RIPv2_MIN_AUTH_PACKET_BYTES) {
	set_error(c_format("packet too small (%u bytes)",
			   XORP_UINT_CAST(packet_bytes)));
	return false;
    }

    const MD5PacketRouteEntry4* mpr =
	reinterpret_cast<const MD5PacketRouteEntry4*>(packet +
						      sizeof(RipPacketHeader));

    if (mpr->addr_family() != MD5PacketRouteEntry4::ADDR_FAMILY) {
	set_error("not an authenticated packet");
	return false;
    }

    if (mpr->auth_type() != MD5PacketRouteEntry4::AUTH_TYPE) {
	set_error("not an MD5 authenticated packet");
	return false;
    }

    if (mpr->auth_bytes() != sizeof(MD5PacketTrailer)) {
	set_error(c_format("wrong number of auth bytes (%d != %u)",
			   mpr->auth_bytes(),
			   XORP_UINT_CAST(sizeof(MD5PacketTrailer))));
	return false;
    }

    if (uint32_t(mpr->auth_offset() + mpr->auth_bytes()) != packet_bytes) {
	set_error(c_format("Size of packet does not correspond with "
			   "authentication data offset and size "
			   "(%d + %d != %u).", mpr->auth_offset(),
			   mpr->auth_bytes(),
			   XORP_UINT_CAST(packet_bytes)));
	return false;
    }

    KeyChain::iterator k = find_if(_valid_key_chain.begin(),
				   _valid_key_chain.end(),
				   bind2nd(mem_fun_ref(&MD5Key::id_matches),
					   mpr->key_id()));
    if (k == _valid_key_chain.end()) {
	set_error(c_format("packet with key ID %d for which no key is "
			   "configured", mpr->key_id()));
	return false;
    }
    MD5Key* key = &(*k);

    if (new_peer)
	key->reset(src_addr);

    uint32_t last_seqno_recv = key->last_seqno_recv(src_addr);
    if (key->packets_received(src_addr) && !(new_peer && mpr->seqno() == 0) &&
	(mpr->seqno() - last_seqno_recv >= 0x7fffffff)) {
	set_error(c_format("bad sequence number 0x%08x < 0x%08x",
			   XORP_UINT_CAST(mpr->seqno()),
			   XORP_UINT_CAST(last_seqno_recv)));
	return false;
    }

    const MD5PacketTrailer* mpt =
	reinterpret_cast<const MD5PacketTrailer*>(packet +
						  mpr->auth_offset());
    if (mpt->valid() == false) {
	set_error("invalid authentication trailer");
	return false;
    }

    MD5_CTX ctx;
    uint8_t digest[16];

    MD5_Init(&ctx);
    MD5_Update(&ctx, packet, mpr->auth_offset() + mpt->data_offset());
    MD5_Update(&ctx, key->key_data(), key->key_data_bytes());
    MD5_Final(digest, &ctx);

    if (memcmp(digest, mpt->data(), mpt->data_bytes()) != 0) {
	set_error(c_format("authentication digest doesn't match local key "
			   "(key ID = %d)", key->id()));
// #define	DUMP_BAD_MD5
#ifdef	DUMP_BAD_MD5
	const char badmd5[] = "/tmp/rip_badmd5";
	// If the file already exists don't dump anything. The file
	// should contain and only one packet.
	if (-1 == access(badmd5, R_OK)) {
	    XLOG_INFO("Dumping bad MD5 to %s", badmd5);
	    FILE *fp = fopen(badmd5, "w");
	    fwrite(packet, packet_bytes, 1 , fp);
	    fclose(fp);
	}
#endif
	return false;
    }

    // Update sequence number only after packet has passed digest check
    key->set_last_seqno_recv(src_addr, mpr->seqno());

    reset_error();
    n_entries = (mpr->auth_offset() - sizeof(RipPacketHeader)) /
	sizeof(PacketRouteEntry<IPv4>) - 1;

    if (n_entries > 0)
	entries = reinterpret_cast<const PacketRouteEntry<IPv4>*>(mpr + 1);

    return true;
}

bool
MD5AuthHandler::authenticate_outbound(RipPacket<IPv4>&	packet,
				      list<RipPacket<IPv4> *>& auth_packets,
				      size_t&		n_routes)
{
    RipPacket<IPv4> first_packet(packet);
    vector<uint8_t> first_trailer;
    KeyChain::iterator iter;

    static_assert(sizeof(MD5PacketTrailer) == 20);

    //
    // XXX: if no valid keys, then don't use any authentication
    //
    if (_valid_key_chain.empty()) {
	if (_null_handler.authenticate_outbound(packet, auth_packets,
						n_routes)
	    != true) {
	    set_error(_null_handler.error());
	    return (false);
	}
	reset_error();
	return (true);
    }

    //
    // Create an authenticated copy of the packet for each valid key
    //
    for (iter = _valid_key_chain.begin();
	 iter != _valid_key_chain.end();
	 ++iter) {
	MD5Key& key = *iter;

	RipPacket<IPv4>* copy_packet = new RipPacket<IPv4>(packet);
	auth_packets.push_back(copy_packet);

	PacketRouteEntry<IPv4>* first_entry = NULL;
	if (head_entries() > 0)
	    first_entry = copy_packet->route_entry(0);

	MD5PacketRouteEntry4* mpr =
	    reinterpret_cast<MD5PacketRouteEntry4*>(first_entry);

	mpr->initialize(copy_packet->data_bytes(), key.id(),
			sizeof(MD5PacketTrailer),
			key.next_seqno_out());

	vector<uint8_t> trailer;
	trailer.resize(sizeof(MD5PacketTrailer));
	MD5PacketTrailer* mpt =
	    reinterpret_cast<MD5PacketTrailer*>(&trailer[0]);
	mpt->initialize();

	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, copy_packet->data_ptr(), mpr->auth_offset());
	MD5_Update(&ctx, &trailer[0], mpt->data_offset());
	MD5_Update(&ctx, key.key_data(), key.key_data_bytes());
	MD5_Final(mpt->data(), &ctx);

	//
	// XXX: create a copy of the first packet without the trailer
	// and of the trailer itself.
	//
	if (iter == _valid_key_chain.begin()) {
	    first_packet = *copy_packet;
	    first_trailer = trailer;
	}

	copy_packet->append_data(trailer);
    }

    packet = first_packet;
    n_routes = packet.data_bytes() / sizeof(MD5PacketRouteEntry4) - 1;
    packet.append_data(first_trailer);

    reset_error();

    return (true);
}

bool
MD5AuthHandler::add_key(uint8_t		key_id,
			const string&	key,
			const TimeVal&	start_timeval,
			const TimeVal&	end_timeval,
			string&		error_msg)
{
    TimeVal now;
    XorpTimer start_timer, end_timer;
    string dummy_error_msg;

    _eventloop.current_time(now);

    if (start_timeval > end_timeval) {
	error_msg = c_format("Start time is later than the end time");
	return false;
    }
    if (end_timeval < now) {
	error_msg = c_format("End time is in the past");
	return false;
    }

    if (start_timeval > now) {
	start_timer = _eventloop.new_oneoff_at(
	    start_timeval,
	    callback(this, &MD5AuthHandler::key_start_cb, key_id));
    }

    if (end_timeval != TimeVal::MAXIMUM()) {
	end_timer = _eventloop.new_oneoff_at(
	    end_timeval,
	    callback(this, &MD5AuthHandler::key_stop_cb, key_id));
    }

    //
    // XXX: If we are using the last authentication key that has expired,
    // move it to the list of invalid keys.
    //
    if (_valid_key_chain.size() == 1) {
	MD5Key& key = _valid_key_chain.front();
	if (key.is_persistent()) {
	    key.set_persistent(false);
	    _invalid_key_chain.push_back(key);
	    _valid_key_chain.pop_front();
	}
    }

    // XXX: for simplicity just try to remove the key even if it doesn't exist
    remove_key(key_id, dummy_error_msg);

    // Add the new key to the appropriate chain
    MD5Key new_key = MD5Key(key_id, key, start_timeval, end_timeval,
			    start_timer, end_timer);
    if (start_timer.scheduled())
	_invalid_key_chain.push_back(new_key);
    else
	_valid_key_chain.push_back(new_key);

    return true;
}

bool
MD5AuthHandler::remove_key(uint8_t key_id, string& error_msg)
{
    KeyChain::iterator i;

    // Check among all valid keys
    i = find_if(_valid_key_chain.begin(), _valid_key_chain.end(),
		bind2nd(mem_fun_ref(&MD5Key::id_matches), key_id));
    if (i != _valid_key_chain.end()) {
	_valid_key_chain.erase(i);
	return true;
    }

    // Check among all invalid keys
    i = find_if(_invalid_key_chain.begin(), _invalid_key_chain.end(),
		bind2nd(mem_fun_ref(&MD5Key::id_matches), key_id));
    if (i != _invalid_key_chain.end()) {
	_invalid_key_chain.erase(i);
	return true;
    }

    error_msg = c_format("No such key");
    return false;
}

void
MD5AuthHandler::key_start_cb(uint8_t key_id)
{
    KeyChain::iterator i;

    // Find the key among all invalid keys and move it to the valid keys
    i = find_if(_invalid_key_chain.begin(), _invalid_key_chain.end(),
		bind2nd(mem_fun_ref(&MD5Key::id_matches), key_id));
    if (i != _invalid_key_chain.end()) {
	MD5Key& key = *i;
	_valid_key_chain.push_back(key);
	_invalid_key_chain.erase(i);
    }
}

void
MD5AuthHandler::key_stop_cb(uint8_t key_id)
{
    KeyChain::iterator i;

    // Find the key among all valid keys and move it to the invalid keys
    i = find_if(_valid_key_chain.begin(), _valid_key_chain.end(),
		bind2nd(mem_fun_ref(&MD5Key::id_matches), key_id));
    if (i != _valid_key_chain.end()) {
	MD5Key& key = *i;
	//
	// XXX: If the last key expires then keep using it as per
	// RFC 2082 Section 4.3 until the lifetime is extended, the key
	// is deleted by network management, or a new key is configured.
	//
	if (_valid_key_chain.size() == 1) {
	    XLOG_WARNING("Last authentication key (key ID = %u) has expired. "
			 "Will keep using it until its lifetime is extended, "
			 "the key is deleted, or a new key is configured.",
			 key_id);
	    key.set_persistent(true);
	    return;
	}
	_invalid_key_chain.push_back(key);
	_valid_key_chain.erase(i);
    }
}

void
MD5AuthHandler::reset_keys()
{
    KeyChain::iterator iter;

    for (iter = _valid_key_chain.begin();
	 iter != _valid_key_chain.end();
	 ++iter) {
	iter->reset();
    }
}

bool
MD5AuthHandler::empty() const
{
    return (_valid_key_chain.empty() && _invalid_key_chain.empty());
}
