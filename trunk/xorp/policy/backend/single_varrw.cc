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

#ident "$XORP: xorp/policy/backend/single_varrw.cc,v 1.8 2005/09/04 20:52:32 abittau Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "policy/policy_module.h"
#include "libxorp/xlog.h"
#include "single_varrw.hh"
#include "policy/common/elem_null.hh"

SingleVarRW::SingleVarRW() : _trashc(0), _did_first_read(false) 
{
    bzero(&_elems, sizeof(_elems));
    bzero(&_modified, sizeof(_modified));
}


SingleVarRW::~SingleVarRW() {
    for (unsigned i = 0; i < _trashc; i++)
        delete _trash[i];
}

const Element&
SingleVarRW::read(const Id& id) {

    // Maybe there was a write before a read for this variable, if so, just
    // return the value... no need to bother the client.
    const Element* e = _elems[id];

    // nope... no value found.
    if(!e) {

	// if it's the first read, inform the client.
	if(!_did_first_read) {
	    start_read();
	    _did_first_read = true;

	    // try again, old clients initialize on start_read()
	    e = _elems[id];

	    // no luck... need to explicitly read...
	    if (!e)
		initialize(id, single_read(id));
	}
	// client already had chance to initialize... but apparently didn't...
	else
	   initialize(id, single_read(id));

	// the client may have initialized the variables after the start_read
	// marker, so try reading again...
	e = _elems[id];

	// out of luck...
	if(!e)
	    throw SingleVarRWErr("Unable to read variable " + id);
    }

    return *e;
}

void
SingleVarRW::write(const Id& id, const Element& e) {
    // XXX no paranoid checks on what we write

    _elems[id] = &e;
    _modified[id] = true;
}

void
SingleVarRW::sync() {
    bool first = true;

    // it's faster doing it this way rather than STL set if VAR_MAX is small...
    for (unsigned i = 0; i < VAR_MAX; i++) {
	if (!_modified[i])
	    continue;

	if (first) {
	    // alert derived class we are committing
	    start_write();
	    first = false;
	}    
	const Element* e = _elems[i];

	XLOG_ASSERT(e);
	single_write(i,*e);
	_modified[i] = false;
    }
    
    // done commiting [so the derived class may sync]
    end_write();

    // clear cache
    bzero(&_elems, sizeof(_elems));
    
    // delete all garbage
    for (unsigned i = 0; i < _trashc; i++)
        delete _trash[i];
    _trashc = 0;
}

void
SingleVarRW::initialize(const Id& id, Element* e) {

    // check if we already have a value for a variable.
    // if so, do nothing.
    //
    // Consider clients initializing variables on start_read.
    // Consider a variable being written to before any reads. In such a case, the
    // SingleVarRW will already have the correct value for that variable, so we
    // need to ignore any initialize() called for that variable.
    if(_elems[id]) {
	if(e)
	    delete e;
	return;
    }

    // special case nulls [for supported variables, but not present in this
    // particular case].
    if(!e)
	e = new ElemNull();
    
    _elems[id] = e;

    // we own the pointers.
    XLOG_ASSERT(_trashc < sizeof(_trash)/sizeof(Element*));
    _trash[_trashc] = e;
    _trashc++;
}
