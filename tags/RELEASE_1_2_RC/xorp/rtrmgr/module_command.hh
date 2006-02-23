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

// $XORP: xorp/rtrmgr/module_command.hh,v 1.23 2005/07/11 21:49:29 pavlin Exp $

#ifndef __RTRMGR_MODULE_COMMAND_HH__
#define __RTRMGR_MODULE_COMMAND_HH__


#include "template_commands.hh"


class TaskManager;
class TemplateTreeNode;
class Validation;
class MasterConfigTreeNode;

class ModuleCommand : public Command {
public:
    ModuleCommand(TemplateTree& template_tree,
		  TemplateTreeNode& template_tree_node,
		  const string& cmd_name);
    ~ModuleCommand();

    void add_action(const list<string>& action,
		    const XRLdb& xrldb) throw (ParseError);
    virtual bool expand_actions(string& error_msg);
    virtual bool check_referred_variables(string& error_msg) const;

    Validation* startup_validation(TaskManager& taskmgr) const;
    Validation* config_validation(TaskManager& taskmgr) const;
    Validation* ready_validation(TaskManager& taskmgr) const;
    Validation* shutdown_validation(TaskManager& taskmgr) const;
    Startup*	startup_method(TaskManager& taskmgr) const;
    Shutdown*	shutdown_method(TaskManager& taskmgr) const;

    const string& module_name() const { return _module_name; }
    const string& module_exec_path() const { return _module_exec_path; }
    const list<string>& depends() const { return _depends; }
    int start_transaction(MasterConfigTreeNode& ctn,
			  TaskManager& task_manager) const;
    int end_transaction(MasterConfigTreeNode& ctn,
			TaskManager& task_manager) const;
    string str() const;

    typedef XorpCallback3<void, bool, const string&, const string&>::RefPtr ProgramCallback;

protected:
    void xrl_action_complete(const XrlError& err,
			     XrlArgs* xrl_args,
			     MasterConfigTreeNode *ctn,
			     Action *action,
			     string cmd) const;

    void program_action_complete(bool success,
				 const string& stdout_output,
				 const string& stderr_output,
				 bool do_exec,
				 MasterConfigTreeNode *ctn,
				 Action *action,
				 string cmd) const;

private:
    TemplateTree&	_tt;
    string		_module_name;
    string		_module_exec_path;
    string		_default_target_name;
    list<string>	_depends;
    Action*		_start_commit;
    Action*		_end_commit;
    Action*		_status_method;
    Action*		_startup_method;
    Action*		_shutdown_method;
    bool		_execute_done;
    bool		_verbose;	// Set to true if output is verbose
};

#endif // __RTRMGR_MODULE_COMMAND_HH__
