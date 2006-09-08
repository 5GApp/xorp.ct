// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
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

// $XORP: xorp/policy/term.hh,v 1.12 2006/05/12 02:21:38 pavlin Exp $

#ifndef __POLICY_TERM_HH__
#define __POLICY_TERM_HH__

#include "libproto/config_node_id.hh"
#include "policy/common/policy_exception.hh"

#include <map>
#include <string>
#include "node_base.hh"


/**
 * @short A term is an atomic policy unit. 
 *
 * It is a complete specification of how a route needs to be matched, and what
 * actions must be taken.
 */
class Term {
public:
    enum BLOCKS {
	SOURCE = 0,
	DEST,
	ACTION,

	// keep this last
	LAST_BLOCK
    };

    // the integer is the "line number", the node is the parsed structure [AST]
    // of the statement(s) in that line.
    typedef ConfigNodeIdMap<Node*> Nodes;

    /**
     * @short Exception thrown on a syntax error while parsing configuration.
     */
    class term_syntax_error :  public PolicyException {
    public:
        term_syntax_error(const char* file, size_t line, 
			  const string& init_why = "")   
            : PolicyException("term_syntax_error", file, line, init_why) {}  
    };

    /**
     * @param name term name.
     */
    Term(const string& name);
    ~Term();
   
    /**
     * @return name of the term.
     */
    const string& name() const { return _name; }
    
    /**
     * Perform operations at the end of the term.
     */
    void set_term_end();

    /**
     * Updates the source/dest/action block of a term.
     *
     * @param block the block to update (0:source, 1:dest, 2:action).
     * @param order node ID with position of term.
     * @param statement the statement to insert.
     */
    void set_block(const uint32_t& block, const ConfigNodeId& order,
		   const string& statement);

    /**
     * Deletes statements in the location specified by order and block.
     *
     * @param block the block to update (0:source, 1:dest, 2:action).
     * @param order node ID with position of term.
     */
    void del_block(const uint32_t& block, const ConfigNodeId& order);

    /**
     * Perform operations at the end of the block.
     *
     * @param block the block to perform operations on
     * (0:source, 1:dest, 2:action).
     */
    void set_block_end(uint32_t block);

    /**
     * Visitor implementation.
     *
     * @param v visitor used to visit this term.
     */
    const Element* accept(Visitor& v) {
	return v.visit(*this);
    }

    /**
     * @return parse tree of source block.
     */
    Nodes& source_nodes() { return *_source_nodes; }

    /**
     * @return parse tree of dest block.
     */
    Nodes& dest_nodes() { return *_dest_nodes; }

    /**
     * @return parse tree of action block.
     */
    Nodes& action_nodes() { return *_action_nodes; }

    /**
     * Convert block number to human readable form.
     *
     * @param num the block number.
     * @return human readable representation of block name.
     */
    static string block2str(uint32_t num); 

private:
    list<pair<ConfigNodeId, Node*> >::iterator find_out_of_order_node(
	const uint32_t& block, const ConfigNodeId& order);

    string _name;

    Nodes* _block_nodes[3];
    list<pair<ConfigNodeId, Node*> > _out_of_order_nodes[3];

    Nodes*& _source_nodes; 
    Nodes*& _dest_nodes;
    Nodes*& _action_nodes;

    // not impl
    Term(const Term&);
    Term& operator=(const Term&);
};

#endif // __POLICY_TERM_HH__
