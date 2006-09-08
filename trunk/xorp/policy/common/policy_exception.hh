// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/common/policy_exception.hh,v 1.3 2006/03/16 00:05:19 pavlin Exp $

#ifndef __POLICY_COMMON_POLICY_EXCEPTION_HH__
#define __POLICY_COMMON_POLICY_EXCEPTION_HH__

#include <string>
#include "libxorp/exceptions.hh"


/**
 * @short Base class for all policy exceptions.
 *
 * All policy exceptions have a string representing the error.
 */
/**
 * @short Base class for all policy exceptions.
 *
 * All policy exceptions have a string representing the error.
 */
class PolicyException : public XorpReasonedException {
public:
    /**
     * @param reason the error message
     */
    PolicyException(const char* file, size_t line, 
			const string& init_why = "")   
      : XorpReasonedException("PolicyException", file, line, init_why) {} 

    PolicyException(const char* type, const char* file, size_t line, 
			const string& init_why = "")   
      : XorpReasonedException(type, file, line, init_why) {} 
    virtual ~PolicyException() {}
};


#endif // __POLICY_COMMON_POLICY_EXCEPTION_HH__
