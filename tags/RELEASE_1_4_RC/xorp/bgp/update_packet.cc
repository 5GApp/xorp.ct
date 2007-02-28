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

#ident "$XORP: xorp/bgp/update_packet.cc,v 1.42 2006/10/12 01:24:40 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "packet.hh"


#if 1
void
dump_bytes(const uint8_t *d, size_t l)
{
        printf("DEBUG_BYTES FN : %p %u\n", d, XORP_UINT_CAST(l));
	for (u_int i=0;i<l;i++)
	    printf("%x ",*((const char *)d + i));
	printf("\n");
}
#else
void dump_bytes(const uint8_t*, size_t) {}
#endif

/* **************** UpdatePacket *********************** */

UpdatePacket::UpdatePacket()
{
    _Type = MESSAGETYPEUPDATE;
}

UpdatePacket::~UpdatePacket()
{
}

void
UpdatePacket::add_nlri(const BGPUpdateAttrib& nlri)
{
    _nlri_list.push_back(nlri);
}

void
UpdatePacket::add_pathatt(const PathAttribute& pa)
{
    _pa_list.add_path_attribute(pa);
//    _pa_list.push_back(paclone);
}

void
UpdatePacket::add_pathatt(PathAttribute *pa)
{
    _pa_list.add_path_attribute(pa);
//     _pa_list.push_back(pa);
}

void
UpdatePacket::add_withdrawn(const BGPUpdateAttrib& wdr)
{
    _wr_list.push_back(wdr);
}

bool 
UpdatePacket::big_enough() const
{
    /* is the packet now large enough that we'd be best to send the
       part we already have before it exceeds the 4K size limit? */
    //XXXX needs additional tests for v6

    //quick and dirty check
    if (((_wr_list.size() + _nlri_list.size())* 4) > 2048) {
	debug_msg("withdrawn size = %u\n", XORP_UINT_CAST(_wr_list.size()));
	debug_msg("nlri size = %u\n", XORP_UINT_CAST(_wr_list.size()));
	return true;
    }
    return false;
}

const uint8_t *
UpdatePacket::encode(size_t &len, uint8_t *d) const
{
    XLOG_ASSERT( _nlri_list.empty() ||  ! pa_list().empty());

    list <PathAttribute*>::const_iterator pai;
    size_t i, pa_len = 0;
    size_t wr_len = wr_list().wire_size();
    size_t nlri_len = nlri_list().wire_size();

    // compute packet length

    for (pai = pa_list().begin() ; pai != pa_list().end(); ++pai)
	pa_len += (*pai)->wire_size();

    size_t desired_len = BGPPacket::MINUPDATEPACKET + wr_len + pa_len
	+ nlri_len;
    if (d != 0)		// XXX have a buffer, check length
	assert(len >= desired_len);	// XXX should throw an exception
    len = desired_len;

    if (len > BGPPacket::MAXPACKETSIZE)		// XXX
	XLOG_FATAL("Attempt to encode a packet that is too big");

    debug_msg("Path Att: %u Withdrawn Routes: %u Net Reach: %u length: %u\n",
	      XORP_UINT_CAST(pa_list().size()),
	      XORP_UINT_CAST(_wr_list.size()),
	      XORP_UINT_CAST(_nlri_list.size()),
	      XORP_UINT_CAST(len));
    d = basic_encode(len, d);	// allocate buffer and fill header

    // fill withdraw list length (bytes)
    d[BGPPacket::COMMON_HEADER_LEN] = (wr_len >> 8) & 0xff;
    d[BGPPacket::COMMON_HEADER_LEN + 1] = wr_len & 0xff;

    // fill withdraw list
    i = BGPPacket::COMMON_HEADER_LEN + 2;
    wr_list().encode(wr_len, d + i);
    i += wr_len;

    // fill pathattribute length (bytes)
    d[i++] = (pa_len >> 8) & 0xff;
    d[i++] = pa_len & 0xff;

    // fill path attribute list
    for (pai = pa_list().begin() ; pai != pa_list().end(); ++pai) {
        memcpy(d+i, (*pai)->data(), (*pai)->wire_size());
	i += (*pai)->wire_size();
    }	

    // fill NLRI list
    nlri_list().encode(nlri_len, d+i);
    i += nlri_len;
    return d;
}

UpdatePacket::UpdatePacket(const uint8_t *d, uint16_t l) 
    throw(CorruptMessage)
{
    debug_msg("UpdatePacket constructor called\n");
    _Type = MESSAGETYPEUPDATE;
    if (l < BGPPacket::MINUPDATEPACKET)
	xorp_throw(CorruptMessage,
		   c_format("Update Message too short %d", l),
		   MSGHEADERERR, BADMESSLEN, d + BGPPacket::MARKER_SIZE, 2);
    d += BGPPacket::COMMON_HEADER_LEN;		// move past header
    size_t wr_len = (d[0] << 8) + d[1];		// withdrawn length

    if (BGPPacket::MINUPDATEPACKET + wr_len > l)
	xorp_throw(CorruptMessage,
		   c_format("Unreachable routes length is bogus %u > %u",
			    XORP_UINT_CAST(wr_len),
			    XORP_UINT_CAST(l - BGPPacket::MINUPDATEPACKET)),
		   UPDATEMSGERR, MALATTRLIST);
    
    size_t pa_len = (d[wr_len+2] << 8) + d[wr_len+3];	// pathatt length
    if (BGPPacket::MINUPDATEPACKET + pa_len + wr_len > l)
	xorp_throw(CorruptMessage,
		   c_format("Pathattr length is bogus %u > %u",
			    XORP_UINT_CAST(pa_len),
			    XORP_UINT_CAST(l - wr_len - BGPPacket::MINUPDATEPACKET)),
		UPDATEMSGERR, MALATTRLIST);

    size_t nlri_len = l - BGPPacket::MINUPDATEPACKET - pa_len - wr_len;

    // Start of decoding of withdrawn routes.
    d += 2;	// point to the routes.
    _wr_list.decode(d, wr_len);
    d += wr_len;

    // Start of decoding of Path Attributes
    d += 2; // move past Total Path Attributes Length field
	
    while (pa_len > 0) {
	size_t used = 0;
        PathAttribute *pa = PathAttribute::create(d, pa_len, used);
	debug_msg("attribute size %u\n", XORP_UINT_CAST(used));
	if (used == 0)
	    xorp_throw(CorruptMessage,
		   c_format("failed to read path attribute"),
		   UPDATEMSGERR, ATTRLEN);

	add_pathatt(pa);
// 	_pa_list.push_back(pa);
	d += used;
	pa_len -= used;
    }

    // Start of decoding of Network Reachability
    _nlri_list.decode(d, nlri_len);
    /* End of decoding of Network Reachability */
    debug_msg("No of withdrawn routes %u. No of path attributes %u. "
	      "No of networks %u.\n",
	      XORP_UINT_CAST(_wr_list.size()),
	      XORP_UINT_CAST(pa_list().size()),
	      XORP_UINT_CAST(_nlri_list.size()));
}

string
UpdatePacket::str() const
{
    string s = "Update Packet\n";
    debug_msg("No of withdrawn routes %u. No of path attributes %u. "
		"No of networks %u.\n",
	      XORP_UINT_CAST(_wr_list.size()),
	      XORP_UINT_CAST(pa_list().size()),
	      XORP_UINT_CAST(_nlri_list.size()));

    s += _wr_list.str("Withdrawn");

    list <PathAttribute*>::const_iterator pai = pa_list().begin();
    while (pai != pa_list().end()) {
	s = s + " - " + (*pai)->str() + "\n";
	++pai;
    }
    
    s += _nlri_list.str("Nlri");
    return s;
}

/*
 * Helper function used when sorting Path Attribute lists.
 */
inline
bool
compare_path_attributes(PathAttribute *a, PathAttribute *b)
{
    return *a < *b;
}

bool 
UpdatePacket::operator==(const UpdatePacket& him) const 
{
    debug_msg("compare %s and %s", this->str().c_str(), him.str().c_str());

    if (_wr_list != him.wr_list())
	return false;

    //path attribute equals
    list <PathAttribute *> temp_att_list(pa_list());
    temp_att_list.sort(compare_path_attributes);
    list <PathAttribute *> temp_att_list_him(him.pa_list());
    temp_att_list_him.sort(compare_path_attributes);

    if (temp_att_list.size() != temp_att_list_him.size())
	return false;

    list <PathAttribute *>::const_iterator pai = temp_att_list.begin();
    list <PathAttribute *>::const_iterator pai_him = temp_att_list_him.begin();
    while (pai != temp_att_list.end() && pai_him != temp_att_list_him.end()) {
	
	if ( (**pai) == (**pai_him) ) {
	    ++pai;
	    ++pai_him;
	} else  {
	    debug_msg("%s did not match %s\n", (*pai)->str().c_str(),
		      (*pai_him)->str().c_str());
	    return false;
	}
    }

    //net layer reachability equals
    if (_nlri_list != him.nlri_list())
	return false;

    return true;
}

