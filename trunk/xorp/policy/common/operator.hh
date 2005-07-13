// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/common/operator.hh,v 1.3 2005/07/12 22:07:24 abittau Exp $

#ifndef __POLICY_COMMON_OPERATOR_HH__
#define __POLICY_COMMON_OPERATOR_HH__

#include "operator_base.hh"

#define DEFINE_UNOPER(name,human) \
class name : public UnOper { \
public: \
    ~name() {} \
    string str() const { return #human; } \
}; 

// Macro ugliness
// name is the name of the class/operation
// Human is the human readable version [str()]
#define DEFINE_BINOPER(name,human) \
class name : public BinOper { \
public: \
    ~name() {} \
    string str() const { return #human; } \
}; 

// Logical operators
DEFINE_BINOPER(OpAnd,AND)
DEFINE_BINOPER(OpOr,OR)
DEFINE_BINOPER(OpXor,XOR)

// Relational operators
DEFINE_BINOPER(OpEq,==)
DEFINE_BINOPER(OpNe,!=)
DEFINE_BINOPER(OpLt,<)
DEFINE_BINOPER(OpGt,>)
DEFINE_BINOPER(OpLe,<=)
DEFINE_BINOPER(OpGe,>=)

// Math operators
DEFINE_BINOPER(OpAdd,+)
DEFINE_BINOPER(OpSub,-)
DEFINE_BINOPER(OpMul,*)

// Regular expression operator
DEFINE_BINOPER(OpRegex,REGEX)

DEFINE_BINOPER(OpCtr,CTR)

DEFINE_BINOPER(OpNEInt,NON_EMPTY_INTERSECTION)

// Unary operators
DEFINE_UNOPER(OpNot,NOT)
DEFINE_UNOPER(OpHead,HEAD)

#endif // __POLICY_COMMON_OPERATOR_HH__
