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

#ident "$XORP: xorp/fea/fticonfig_entry_set.cc,v 1.11 2007/02/16 22:45:38 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fibconfig.hh"
#include "fibconfig_entry_set.hh"


//
// Set single-entry information into the unicast forwarding table.
//


FtiConfigEntrySet::FtiConfigEntrySet(FtiConfig& ftic)
    : _is_running(false),
      _ftic(ftic),
      _in_configuration(false),
      _is_primary(true)
{
    
}

FtiConfigEntrySet::~FtiConfigEntrySet()
{
    
}

void
FtiConfigEntrySet::register_ftic_primary()
{
    _ftic.register_ftic_entry_set_primary(this);
}

void
FtiConfigEntrySet::register_ftic_secondary()
{
    _ftic.register_ftic_entry_set_secondary(this);

    //
    // XXX: push the current config into the new secondary
    //
    if (_is_running) {
	// XXX: nothing to do. 
	//
	// XXX: note that the forwarding table state in the secondary methods
	// is pushed by FtiConfigTableSet::register_ftic_secondary(), hence
	// we don't need to do it here again. However, this is based on the
	// assumption that for each FtiConfigEntrySet secondary method
	// there is a corresponding FtiConfigTableSet secondary method.
	//
    }
}
