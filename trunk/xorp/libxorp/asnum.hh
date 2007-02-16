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

// $XORP: xorp/libxorp/asnum.hh,v 1.14 2006/10/12 01:24:50 pavlin Exp $

#ifndef __LIBXORP_ASNUM_HH__
#define __LIBXORP_ASNUM_HH__

#include "libxorp/xorp.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "c_format.hh"


/**
 * @short A class for storing an AS number used by protocols such as BGP
 * 
 * This class can be used to store an AS number that can be either 16
 * or 32 bits.  Originally, the AS numbers were defined as 16-bit
 * unsigned numbers.  Later the "extended" AS numbers were introduced,
 * which are unsigned 32-bit numbers.  Conventional terminology refers
 * to the 32-bit version as 4-byte AS numbers rather than 32-bit AS
 * numbers, so we'll try and stick with that where it makes sense.
 *
 * 2-byte numbers are expanded to 32-bits by extending them with 0's in front.
 * 4-byte numbers are represented in a 2-byte AS path, by a special
 * 16-bit value, AS_TRAN, which will be allocated by IANA.
 * Together with any AsPath containing AS_TRAN, we will always see a
 * AS4_PATH attribute which contains the full 32-bit representation
 * of the path.  So there is no loss of information.
 *
 * IANA refers to NEW_AS_PATH, but the latest internet drafts refer to
 * AS4_PATH.  They're the same thing, but I the latter is preferred so
 * we'll use that.
 *
 * The internal representation of an AsNum is 32-bit in host order.
 *
 * The canonical string form of a 4-byte AS number is <high>.<low>, so
 * decimal 65536 ends up being printed as "1.0".
 *
 * An AsNum must always be initialized, so the default constructor
 * is never called.
 */
class AsNum {
public:
    static const uint16_t AS_INVALID = 0;	// XXX IANA-reserved
    static const uint16_t AS_TRAN = 23456;	// IANA

    /**
     * Constructor.
     * @param value the value to assign to this AS number.
     */
    explicit AsNum(const uint32_t value) : _as(value)	{
    }
 
    explicit AsNum(const uint16_t value) : _as(value)	{}

    explicit AsNum(int value)				{
	assert(value >= 0 && value <= 0xffff);
	_as = value;
    }

    /**
     * construct from a 2-byte buffer in memory
     */
    explicit AsNum(const uint8_t *d)			{
	_as = (d[0] << 8) + d[1];
    }

    /**
     * construct from a 2-byte buffer in memory or a 4 byte buffer (in
     * net byte order).
     *
     * The 4byte parameter is mostly to distinguish this from the
     * 2-byte constructor above.
     */
    explicit AsNum(const uint8_t *d, bool fourbyte)	{
	if (fourbyte) {
	    // 4 byte version
	    memcpy(&_as, d, 4);
	    _as = htonl(_as);
	} else {
	    // 2 byte version
	    _as = (d[0] << 8) + d[1];
	}
    }

    /**
     * Get the non-extended AS number value.
     * 
     * @return the non-extended AS number value.
     */
    uint16_t as() const					{
	return extended() ? AS_TRAN : _as;
    }

    /**
     * Get the extended AS number value.
     * 
     * @return the extended AS number value.
     */
    uint32_t as4() const				{ return _as; }

    /**
     * copy the 16-bit value into a 2-byte memory buffer
     */
    void copy_out(uint8_t *d) const			{
	uint16_t x = as();
	d[0] = (x >> 8) & 0xff;
	d[1] = x & 0xff;
    }

    /**
     * copy the 32-bit value into a 4-byte network byte order memory buffer
     */
    void copy_out4(uint8_t *d) const			{
	// note - buffer may be unaligned, so use memcpy
	uint32_t x = htonl(_as);
	memcpy(d, &x, 4);
    }

    /**
     * Test if this is an extended AS number.
     * 
     * @return true if this is an extended AS number.
     */
    bool extended() const				{ return _as>0xffff;};
    
    /**
     * Equality Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const AsNum& x) const		{ return _as == x._as; }

    /**
     * Less-Than Operator
     * @return true if the left-hand operand is numerically smaller than the
     * right-hand operand.
     */
    bool operator<(const AsNum& x) const		{ return _as < x._as; }
    
    /**
     * Convert this AS number from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the AS number.
     */
    string str() const					{
	if (extended())
	    return c_format("AS/%u.%u", (_as >> 16)&&0xffff, _as&&0xff);
	else 
	    return c_format("AS/%u", XORP_UINT_CAST(_as));
    }

    string short_str() const					{
	if (extended())
	    return c_format("%u.%u", (_as >> 16)&&0xffff, _as&&0xff);
	else
	    return c_format("%u", XORP_UINT_CAST(_as));
    }

    string fourbyte_str() const {
	// this version forces the long canonical format
	return c_format("%u.%u", (_as >> 16)&&0xffff, _as&&0xff);
    }
    
private:
    uint32_t _as;		// The value of the AS number

    AsNum(); // forbidden
};
#endif // __LIBXORP_ASNUM_HH__
