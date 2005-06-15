// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rtrmgr/template_tree_node.hh,v 1.23 2005/03/25 02:54:39 pavlin Exp $

#ifndef __RTRMGR_TEMPLATE_TREE_NODE_HH__
#define __RTRMGR_TEMPLATE_TREE_NODE_HH__


#include <map>
#include <list>
#include <set>
#include <vector>

#include "libxorp/mac.hh"

#include "module_manager.hh"
#include "rtrmgr_error.hh"
#include "xorp_client.hh"
#include "xrldb.hh"


enum TTNodeType {
    NODE_VOID		= 0,
    NODE_TEXT		= 1,
    NODE_UINT		= 2,
    NODE_INT		= 3,
    NODE_BOOL		= 4,
    NODE_TOGGLE		= 4,
    NODE_IPV4		= 5,
    NODE_IPV4NET	= 6,
    NODE_IPV6		= 7,
    NODE_IPV6NET	= 8,
    NODE_MACADDR	= 9
};

enum TTSortOrder {
    ORDER_UNSORTED,
    ORDER_SORTED_NUMERIC,
    ORDER_SORTED_ALPHABETIC
};

class BaseCommand;
class CommandTree;
class TemplateTree;

class TemplateTreeNode {
public:
    TemplateTreeNode(TemplateTree& template_tree, TemplateTreeNode* parent, 
		     const string& path, const string& varname);
    virtual ~TemplateTreeNode();

    virtual TTNodeType type() const { return NODE_VOID; }
    void add_cmd(const string& cmd) throw (ParseError);
    void add_action(const string& cmd, const list<string>& action_list);

    map<string, string> create_variable_map(const list<string>& segments) const;

    virtual string str() const;
    virtual string typestr() const { return string("void"); }
    virtual string default_str() const { return string(""); }
    virtual bool type_match(const string& s) const;
    BaseCommand* command(const string& cmd_name);
    const BaseCommand* const_command(const string& cmd_name) const;
    set<string> commands() const;
    string varname() const { return _varname; }
    void set_tag() { _is_tag = true; }
    bool is_tag() const { return _is_tag; }
    string subtree_str() const;
    TemplateTreeNode* parent() const { return _parent; }
    const list<TemplateTreeNode*>& children() const { return _children; }
    const string& module_name() const { return _module_name; }
    const string& default_target_name() const { return _default_target_name; }
    void set_module_name(const string& module_name) { _module_name = module_name; }
    void set_default_target_name(const string& default_target_name) { _default_target_name = default_target_name; }
    const string& segname() const { return _segname; }
    string path() const;

#if 0
    bool check_template_tree(string& errmsg) const;
#endif
    bool check_command_tree(const list<string>& commands, 
			    bool include_intermediates, size_t depth) const;
    bool has_default() const { return _has_default; }
    bool check_variable_name(const vector<string>& parts, size_t part) const;
    string get_module_name_by_variable(const string& varname) const;
    string get_default_target_name_by_variable(const string& varname) const;
    bool expand_variable(const string& varname, string& value) const;
    bool expand_expression(const string& expression, string& value) const;
    const TemplateTreeNode* find_varname_node(const string& varname) const;

    const list<string>& mandatory_children() const { return _mandatory_children; }
    const string& help() const {
	//if the node is a tag, the help is held on the child.
	if (is_tag()) 
	    return children().front()->help();
	return _help;
    }
    const string& help_long() const {
	if (is_tag()) 
	    return children().front()->help_long();
	if (_help_long == "") 
	    return help();
	return _help_long;
    }

    int child_number() const { return _child_number;}

    TTSortOrder order() const { return _order; }
    void set_order(TTSortOrder o) { _order = o; }

    bool is_deprecated() const { return _is_deprecated; }
    void set_deprecated(bool v) { _is_deprecated = v; }
    const string& deprecated_reason() const { return _deprecated_reason; }
    void set_deprecated_reason(const string& v) { _deprecated_reason = v; }

    /**
     * @return the oldest deprecated ancestor or NULL if no ancestor
     * is deprecated.
     */
    const TemplateTreeNode* find_first_deprecated_ancestor() const;

protected:
    void add_child(TemplateTreeNode* child);

    string strip_quotes(const string& s) const;
    void set_has_default() { _has_default = true; }
    bool name_is_variable() const;

    map<string, BaseCommand *> _cmd_map;
    TemplateTreeNode*	_parent;
    list<TemplateTreeNode*> _children;

private:
    bool split_up_varname(const string& varname,
			  list<string>& var_parts) const;
    const TemplateTreeNode* find_parent_varname_node(const list<string>& var_parts) const;
    const TemplateTreeNode* find_child_varname_node(const list<string>& var_parts) const;

    TemplateTree&	_template_tree;

    string _module_name;
    string _default_target_name;
    string _segname;

    // If this node has a variable name associated with it, _varname is
    // where its stored. Otherwise it is an empty string.
    string _varname;

    // Does the node have a default value?
    bool _has_default;

    // Is the node to be regarded as a tag for it's children rather
    // than a true tree node.
    bool _is_tag;

    // The CLI help information associated with this node.
    string _help;
    string _help_long;

    list<string>	_mandatory_children;

    int _child_number;

    TTSortOrder _order;

    bool		_verbose;	 // Set to true if output is verbose
    bool		_is_deprecated;	// True if node's usage is deprecated
    string		_deprecated_reason; // The reason for deprecation
};

class UIntTemplate : public TemplateTreeNode {
public:
    UIntTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		 const string& path, const string& varname, 
		 const string& initializer) throw (ParseError);

    string typestr() const { return string("uint"); }
    TTNodeType type() const { return NODE_UINT; }
    unsigned int default_value() const { return _default; }
    string default_str() const;
    bool type_match(const string& s) const;

private:
    unsigned int _default;
};

class IntTemplate : public TemplateTreeNode {
public:
    IntTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		const string& path, const string& varname, 
		const string& initializer) throw (ParseError);
    string typestr() const { return string("int"); }
    TTNodeType type() const { return NODE_INT; }
    int default_value() const { return _default; }
    string default_str() const;
    bool type_match(const string& s) const;

private:
    int _default;
};

class TextTemplate : public TemplateTreeNode {
public:
    TextTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		 const string& path, const string& varname, 
		 const string& initializer) throw (ParseError);

    string typestr() const { return string("text"); }
    TTNodeType type() const { return NODE_TEXT; }
    string default_value() const { return _default; }
    string default_str() const { return _default; }
    bool type_match(const string& s) const;

private:
    string _default;
};

class BoolTemplate : public TemplateTreeNode {
public:
    BoolTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		 const string& path, const string& varname, 
		 const string& initializer) throw (ParseError);

    string typestr() const { return string("bool"); }
    TTNodeType type() const { return NODE_BOOL; }
    bool default_value() const { return _default; }
    string default_str() const;
    bool type_match(const string& s) const;

private:
    bool _default;
};

class IPv4Template : public TemplateTreeNode {
public:
    IPv4Template(TemplateTree& template_tree, TemplateTreeNode* parent, 
		 const string& path, const string& varname,
		 const string& initializer) throw (ParseError);
    ~IPv4Template();

    string typestr() const { return string("IPv4"); }
    TTNodeType type() const { return NODE_IPV4; }
    IPv4 default_value() const { return *_default; }
    string default_str() const { return _default->str(); }
    bool type_match(const string& s) const;

private:
    IPv4* _default;
};

class IPv4NetTemplate : public TemplateTreeNode {
public:
    IPv4NetTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		    const string& path, const string& varname, 
		    const string& initializer) throw (ParseError);
    ~IPv4NetTemplate();

    string typestr() const { return string("IPv4Net"); }
    TTNodeType type() const { return NODE_IPV4NET; }
    IPv4Net default_value() const { return *_default; }
    string default_str() const { return _default->str(); }
    bool type_match(const string& s) const;

private:
    IPv4Net* _default;
};

class IPv6Template : public TemplateTreeNode {
public:
    IPv6Template(TemplateTree& template_tree, TemplateTreeNode* parent,
		 const string& path, const string& varname, 
		 const string& initializer) throw (ParseError);
    ~IPv6Template();

    string typestr() const { return string("IPv6"); }
    TTNodeType type() const { return NODE_IPV6; }
    IPv6 default_value() const { return *_default; }
    string default_str() const { return _default->str(); }
    bool type_match(const string& s) const;

private:
    IPv6* _default;
};

class IPv6NetTemplate : public TemplateTreeNode {
public:
    IPv6NetTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		    const string& path, const string& varname, 
		    const string& initializer) throw (ParseError);
    ~IPv6NetTemplate();

    string typestr() const { return string("IPv6Net"); }
    TTNodeType type() const { return NODE_IPV6NET; }
    IPv6Net default_value() const { return *_default; }
    string default_str() const { return _default->str(); }
    bool type_match(const string& s) const;

private:
    IPv6Net* _default;
};

class MacaddrTemplate : public TemplateTreeNode {
public:
    MacaddrTemplate(TemplateTree& template_tree, TemplateTreeNode* parent, 
		    const string& path, const string& varname, 
		    const string& initializer) throw (ParseError);
    ~MacaddrTemplate();

    string typestr() const { return string("macaddr"); }
    TTNodeType type() const { return NODE_MACADDR; }
    Mac default_value() const { return *_default; }
    string default_str() const { return _default->str(); }
    bool type_match(const string& s) const;

private:
    // XXX: really should be a MAC not an EtherMAC, but we'll fix this later
    EtherMac* _default;
};

#endif // __RTRMGR_TEMPLATE_TREE_NODE_HH__
