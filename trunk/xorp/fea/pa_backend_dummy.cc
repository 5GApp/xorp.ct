// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/fea/pa_backend_dummy.cc,v 1.1 2004/12/10 14:40:16 bms Exp $"

#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_backend.hh"
#include "pa_backend_dummy.hh"

/* ------------------------------------------------------------------------- */
/* Constructor and destructor. */

PaDummyBackend::PaDummyBackend()
    throw(PaInvalidBackendException)
{
}

PaDummyBackend::~PaDummyBackend()
{
}

/* ------------------------------------------------------------------------- */
/* General back-end methods. */

const char*
PaDummyBackend::get_name() const
{
    return ("dummy");
}

const char*
PaDummyBackend::get_version() const
{
    return ("0.1");
}

/* ------------------------------------------------------------------------- */
/* IPv4 ACL back-end methods. */

bool
PaDummyBackend::push_entries4(const PaSnapshot4* snap)
{
    // Do nothing.
    UNUSED(snap);
    return true;
}

bool
PaDummyBackend::delete_all_entries4()
{
    // Do nothing.
    return true;
}

const PaBackend::Snapshot4*
PaDummyBackend::create_snapshot4()
{
    return (new PaDummyBackend::Snapshot4());
}

bool
PaDummyBackend::restore_snapshot4(const PaBackend::Snapshot4* snap4)
{
    // Simply check if the snapshot is an instance of our derived snapshot.
    // If it is not, we received it in error, and it is of no interest to us.
    const PaDummyBackend::Snapshot4* dsnap4 =
	dynamic_cast<const PaDummyBackend::Snapshot4*>(snap4);
    XLOG_ASSERT(dsnap4 != NULL);
    return true;
}

/* ------------------------------------------------------------------------- */
/* Snapshot scoped classes (Memento pattern) */

PaDummyBackend::Snapshot4::Snapshot4()
    throw(PaInvalidSnapshotException)
{
}

PaDummyBackend::Snapshot4::Snapshot4(
    const PaDummyBackend::Snapshot4& snap4)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4()
{
    // Nothing to do.
    UNUSED(snap4);
}

PaDummyBackend::Snapshot4::~Snapshot4()
{
}

/* ------------------------------------------------------------------------- */
