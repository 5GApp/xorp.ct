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

#ident "$XORP: xorp/policy/policy_statement.cc,v 1.2 2005/03/25 02:54:08 pavlin Exp $"

#include "policy_module.h"
#include "config.h"
#include "policy_statement.hh"   
#include "policy/common/policy_utils.hh"

using namespace policy_utils;

PolicyStatement::PolicyStatement(const string& name, SetMap& smap) : 
    _name(name), _smap(smap) 
{
}

PolicyStatement::~PolicyStatement()
{
    del_dependancies();
    policy_utils::clear_map(_terms);
}
   
void 
PolicyStatement::add_term(uint32_t order, Term* term)
{
    if(_terms.find(order) != _terms.end()) {
	throw PolicyException("Term already present in position: " +
			      to_str(order));
    }

    _terms[order] = term;
}

PolicyStatement::TermContainer::iterator 
PolicyStatement::get_term_iter(const string& name)
{
    TermContainer::iterator i;

    for(i = _terms.begin();
	i != _terms.end(); ++i) {

        if( (i->second)->name() == name) {
	    return i;
	}
    }    

    return i;
}

PolicyStatement::TermContainer::const_iterator 
PolicyStatement::get_term_iter(const string& name) const 
{
    TermContainer::const_iterator i;

    for(i = _terms.begin();
	i != _terms.end(); ++i) {

        if( (i->second)->name() == name) {
	    return i;
	}
    }    

    return i;
}

Term& 
PolicyStatement::find_term(const string& name) const 
{
    TermContainer::const_iterator i = get_term_iter(name);
    if(i == _terms.end()) 
	throw PolicyStatementErr("Term " + name + " not found in policy " + 
				 _name);


    Term* t = i->second;
    return *t;    
}

bool 
PolicyStatement::delete_term(const string& name)
{
    TermContainer::iterator i = get_term_iter(name);

    if(i == _terms.end())
	return false;

    Term* t = i->second;

    _terms.erase(i);

    delete t;
    return true;
}

const string& 
PolicyStatement::name() const 
{
    return _name; 
}

bool 
PolicyStatement::accept(Visitor& v) 
{
    return v.visit(*this);
}

PolicyStatement::TermContainer& 
PolicyStatement::terms()
{ 
    return _terms; 
}

void 
PolicyStatement::set_dependancy(const set<string>& sets)
{
    // replace all dependancies

    // delete them all
    del_dependancies();

    // replace
    _sets = sets;

    // re-insert dependancies
    for(set<string>::iterator i = _sets.begin(); i != _sets.end(); ++i)
	_smap.add_dependancy(*i,_name);
}

void 
PolicyStatement::del_dependancies() {
    // remove all dependancies
    for(set<string>::iterator i = _sets.begin(); i != _sets.end(); ++i)
	_smap.del_dependancy(*i,_name);

    _sets.clear();    
}

bool
PolicyStatement::term_exists(const string& name) const 
{
    if(get_term_iter(name)  == _terms.end())
	return false;

    return true;
}
