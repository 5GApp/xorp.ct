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

#ident "$XORP: xorp/fea/fibconfig_transaction.cc,v 1.1 2007/04/27 21:11:29 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fibconfig_transaction.hh"


void
FibConfigTransactionManager::unset_error()
{
    _error.erase();
}

bool
FibConfigTransactionManager::set_unset_error(const string& error)
{
    if (_error.empty()) {
	_error = error;
	return true;
    }
    return false;
}

void
FibConfigTransactionManager::pre_commit(uint32_t /* tid */)
{
    string error_msg;

    unset_error();
    if (fibconfig().start_configuration(error_msg) != true) {
	XLOG_ERROR("Cannot start configuration: %s", error_msg.c_str());
	set_unset_error(error_msg);
    }
}

void
FibConfigTransactionManager::post_commit(uint32_t /* tid */)
{
    string error_msg;
    if (fibconfig().end_configuration(error_msg) != true) {
	XLOG_ERROR("Cannot end configuration: %s", error_msg.c_str());
	set_unset_error(error_msg);
    }
}

void
FibConfigTransactionManager::operation_result(bool success,
					      const TransactionOperation& op)
{
    if (success) {
	return;
    }

    const FibConfigTransactionOperation* fto;
    fto = dynamic_cast<const FibConfigTransactionOperation*>(&op);
    if (fto == NULL) {
	//
	// Getting here is programmer error.
	//
	XLOG_ERROR("FTI transaction commit error, \"%s\" is not an FTI "
		   "TransactionOperation",
		   op.str().c_str());
	return;
    }

    //
    // Record error and xlog first error only
    //
    if (set_unset_error(fto->str())) {
	XLOG_ERROR("FTI transaction commit failed on %s",
		   fto->str().c_str());
    }
}
