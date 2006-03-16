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

#ident "$XORP: xorp/policy/var_map.cc,v 1.7 2005/10/02 22:21:51 abittau Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "policy_module.h"
#include "libxorp/debug.h"
#include "var_map.hh"
#include "policy/common/policy_utils.hh"
#include "policy/common/element_factory.hh"

using namespace policy_utils;

const VarMap::VariableMap&
VarMap::variablemap(const string& protocol) const
{

    ProtoMap::const_iterator i = _protocols.find(protocol);
    if(i == _protocols.end()) 
	throw VarMapErr("Unknown protocol: " + protocol);

    const VariableMap* vm = (*i).second;

    return *vm;
}

const VarMap::Variable&
VarMap::variable(const string& protocol, const VarRW::Id& varname) const
{
    const VariableMap& vmap = variablemap(protocol);

    VariableMap::const_iterator i = vmap.find(varname);

    if(i == vmap.end()) {
	ostringstream oss;

	oss << "Unknown variable: " << varname << " in protocol " << protocol;
	throw VarMapErr(oss.str());
    }			

    const Variable* v = (*i).second;

    return *v;
}


VarMap::VarMap(ProcessWatchBase& pw) : _process_watch(pw) 
{
    add_metavariable(new Variable("trace", "u32", WRITE, VarRW::VAR_TRACE));
}

VarMap::~VarMap()
{
    for(ProtoMap::iterator i = _protocols.begin();
	i != _protocols.end(); ++i) {
	
	VariableMap* vm = (*i).second;

	clear_map(*vm);	
    }
    clear_map(_protocols);
    clear_map(_metavars);
}


bool 
VarMap::protocol_known(const string& protocol)
{
    return _protocols.find(protocol) != _protocols.end();
}

void 
VarMap::add_variable(VariableMap& vm, Variable* var)
{
    VariableMap::iterator i = vm.find(var->id);

    if(i != vm.end()) {
	ostringstream oss;

	oss << "Variable " << var->id << " exists already";
	delete var;
	throw VarMapErr(oss.str());
    }	
    
    vm[var->id] = var;
}

void 
VarMap::add_protocol_variable(const string& protocol, Variable* var)
{

    debug_msg("[POLICY] VarMap adding proto: %s, var: %s, type: %s, R/W: %d, ID: %d\n",
	      protocol.c_str(), var->name.c_str(), 
	      var->type.c_str(), var->access, var->id);

    if (!ElementFactory::can_create(var->type)) {
	ostringstream oss;

	oss << "Unable to create element of type: " << var->type
	    << " in proto: " << protocol << " varname: " << var->name;
	delete var;    
	throw VarMapErr(oss.str());
    }

    ProtoMap::iterator iter = _protocols.find(protocol);
    VariableMap* vm;

    // if no variablemap exists for the protocol exists, create one
    if(iter == _protocols.end()) {
        vm = new VariableMap();
        _protocols[protocol] = vm;
    
        _process_watch.add_interest(protocol);

	// add the metavars
	for (MetaVarContainer::iterator i = _metavars.begin(); i !=
	     _metavars.end(); ++i) {
	    
	    Variable* v = i->second;
	    add_variable(*vm, new Variable(*v));
	}
    }
    // or else just update existing one
    else 
        vm = (*iter).second;

    add_variable(*vm, var);

}

void
VarMap::add_metavariable(Variable* v)
{
    if (_metavars.find(v->id) != _metavars.end()) {
	ostringstream oss;

	oss << "Metavar: " << v->id << " exists already" << endl;
	delete v;
	throw VarMapErr(oss.str());
    }

    _metavars[v->id] = v;
}

string
VarMap::str()
{
    ostringstream out;

    // go through protocols
    for (ProtoMap::iterator i = _protocols.begin(); 
	 i != _protocols.end(); ++i) {

	const string& proto = i->first;
	VariableMap* vm = i->second;

	for(VariableMap::iterator j = vm->begin(); j != vm->end(); ++j) {
	    Variable* v = j->second;

	    out << proto << " " << v->name << " " << v->type << " ";
	    if(v->access == READ)
		out << "r";
	    else
		out << "rw";
	    out << endl;	
	}
    }

    return out.str();
}

VarRW::Id
VarMap::var2id(const string& protocol, const string& varname) const
{
    ProtoMap::const_iterator i = _protocols.find(protocol);

    if (i == _protocols.end())
	throw VarMapErr("Unknown protocol: " + protocol);

    const VariableMap* vm = i->second;

    // XXX slow & lame.  Semantic checking / compilation will be slow.
    for (VariableMap::const_iterator j = vm->begin(); j != vm->end(); ++j) {
	const Variable* v = j->second;

	if (v->name == varname)
	    return v->id;
    }

    throw VarMapErr("Unknown variable: " + varname);
}
