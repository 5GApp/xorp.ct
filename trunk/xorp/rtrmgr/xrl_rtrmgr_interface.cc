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

#ident "$XORP: xorp/rtrmgr/xrl_rtrmgr_interface.cc,v 1.31 2004/12/23 22:47:46 mjh Exp $"


#include <sys/stat.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/status_codes.h"

#include "main_rtrmgr.hh"
#include "master_conf_tree.hh"
#include "randomness.hh"
#include "userdb.hh"
#include "xrl_rtrmgr_interface.hh"


XrlRtrmgrInterface::XrlRtrmgrInterface(XrlRouter& r, UserDB& userdb,
				       EventLoop& eventloop,
				       RandomGen& randgen,
				       Rtrmgr& rtrmgr) 
    : XrlRtrmgrTargetBase(&r),
      _xrl_router(r),
      _client_interface(&r),
      _finder_notifier_interface(&r),
      _userdb(userdb),
      _master_config_tree(NULL),
      _eventloop(eventloop), 
      _randgen(randgen),
      _rtrmgr(rtrmgr),
      _exclusive(false),
      _config_locked(false),
      _lock_holder_token(""),
      _verbose(rtrmgr.verbose())
{
}

XrlRtrmgrInterface::~XrlRtrmgrInterface()
{
    multimap<uint32_t, UserInstance*>::iterator iter;
    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	delete iter->second;
    }
}

XrlCmdError
XrlRtrmgrInterface::common_0_1_get_target_name(// Output values,
					       string& name)
{
    name = XrlRtrmgrTargetBase::name();
    return XrlCmdError::OKAY();
}


XrlCmdError
XrlRtrmgrInterface::common_0_1_get_version(// Output values, 
					   string& v)
{
    v = string(version());
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::common_0_1_get_status(// Output values, 
					  uint32_t& status,
					  string& reason)
{
    // XXX placeholder only
    status = PROC_READY;
    reason = "Ready";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::common_0_1_shutdown()
{
    // The rtrmgr does not accept XRL requests to shutdown via this interface.
    return XrlCmdError::COMMAND_FAILED();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_get_pid(// Output values, 
				      uint32_t& pid)
{
    pid = getpid();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_register_client(
	// Input values, 
	const uint32_t&	user_id, 
	const string&	clientname, 
	// Output values, 
	string&	filename,
	uint32_t& pid)
{
    pid = getpid();
    const User* user;
    UserInstance* newuser;

    //
    // Prevent a clientname passing characters that might cause us to
    // overwrite files later.  Prohibiting "/" makes allowing ".." safe.
    //
    if (strchr(clientname.c_str(), '/') != NULL) {
	string err = c_format("Illegal character in clientname: %s", 
			      clientname.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    //
    // Note that clientname is user supplied data, so this would be
    // dangerous if the user could pass "..".
    //
    filename = "/tmp/rtrmgr-" + clientname;
    mode_t oldmode = umask(S_IRWXG|S_IRWXO);
    debug_msg("newmode: %o oldmode: %o\n", S_IRWXG|S_IRWXO, oldmode);

    FILE* file = fopen(filename.c_str(), "w+");
    if (file == NULL) {
	string err = c_format("Failed to create temporary file: %s",
			      filename.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }
    umask(oldmode);

    user = _userdb.find_user_by_user_id(user_id);
    if (user == NULL) {
	unlink(filename.c_str());
	string err = c_format("User ID %d not found", user_id);
	return XrlCmdError::COMMAND_FAILED(err);
    }

    newuser = new UserInstance(user->user_id(), user->username());
    newuser->set_clientname(clientname);
    newuser->set_authtoken(generate_auth_token(user_id, clientname));
    newuser->set_authenticated(false);
    _users.insert(pair<uint32_t, UserInstance*>(user_id,newuser));

    //
    // Be careful here - if for any reason we fail to create, write to,
    // or close the temporary auth file, that's a very different error
    // from if the xorpsh process can't read it correctly.
    //
    if (fprintf(file, "%s", newuser->authtoken().c_str()) 
	< (int)(newuser->authtoken().size())) {
	unlink(filename.c_str());
	string err = c_format("Failed to write to temporary file: %s",
			      filename.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }
    if (fchown(fileno(file), user_id, (gid_t)-1) != 0) {
	unlink(filename.c_str());
	string err = c_format("Failed to chown temporary file %s to user_id %u",
			      filename.c_str(), user_id);
	return XrlCmdError::COMMAND_FAILED(err);
    }
    if (fclose(file) != 0) {
	unlink(filename.c_str());
	string err = c_format("Failed to close temporary file: %s",
			      filename.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_unregister_client(const string& token)
{
    if (!verify_token(token)) {
	string err = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    multimap<uint32_t, UserInstance*>::iterator iter;
    for (iter = _config_users.begin(); iter != _config_users.end(); ++iter) {
	if (iter->second->authtoken() == token) {
	    _config_users.erase(iter);
	    return XrlCmdError::OKAY();
	}
    }
    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	if (iter->second->authtoken() == token) {
	    delete iter->second;
	    _users.erase(iter);
	    return XrlCmdError::OKAY();
	}
    }
    // This cannot happen, because the token wouldn't have verified
    XLOG_UNREACHABLE();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_authenticate_client(
	// Input values, 
	const uint32_t&	user_id, 
	const string&	clientname, 
	const string&	token) {
    UserInstance *user;
    user = find_user_instance(user_id, clientname);
    if (user == NULL) {
	string err = "User Instance not found - did login time out?";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    if (token == user->authtoken()) {
	user->set_authenticated(true);
	initialize_client_state(user_id, user);
	return XrlCmdError::OKAY();
    } else {
	string err = "Bad authtoken";
	return XrlCmdError::COMMAND_FAILED(err);
    }
}

void
XrlRtrmgrInterface::initialize_client_state(uint32_t user_id, 
					    UserInstance *user)
{
    // We need to register interest via the finder in the XRL target
    // associated with this user instance, so that when the user
    // instance goes away without deregistering, we can clean up the
    // state we associated with them.

    XrlFinderEventNotifierV0p1Client::RegisterInstanceEventInterestCB cb; 
    cb = callback(this, &XrlRtrmgrInterface::finder_register_done,
		  user->clientname());
    _finder_notifier_interface.
	send_register_class_event_interest("finder", 
					   _xrl_router.instance_name(),
					   user->clientname(), cb);
    XLOG_TRACE(_verbose, 
	       "registering interest in %s\n", user->clientname().c_str());


    //
    // We need to send the running config and module state to the
    // client, but we first need to return from the current XRL, so we
    // schedule this on a zero-second timer.
    //
    XorpTimer t;
    uint32_t delay = 0;
    if (!_rtrmgr.ready()) {
	delay = 2000;
    }
    t = _eventloop.new_oneoff_after_ms(delay,
             callback(this, &XrlRtrmgrInterface::send_client_state, 
		      user_id, user));
    _background_tasks.push_front(t);
}

void 
XrlRtrmgrInterface::finder_register_done(const XrlError& e, string clientname) 
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to register with finder about XRL %s (err: %s)\n",
		   clientname.c_str(), e.error_msg());
    }
}

void
XrlRtrmgrInterface::send_client_state(uint32_t user_id, UserInstance *user)
{
    debug_msg("send_client_state %s\n", user->clientname().c_str());
    if (!_rtrmgr.ready()) {
	//we're still in the process of reconfiguring, so delay this
	//til later
	initialize_client_state(user_id, user);
	return;
    }

    string config = _master_config_tree->show_tree();
    string client = user->clientname();
    GENERIC_CALLBACK cb2;
    cb2 = callback(this, &XrlRtrmgrInterface::client_updated, 
		   user_id, user);
    debug_msg("Sending config changed to %s\n", client.c_str());
    _client_interface.send_config_changed(client.c_str(),
					  0, config, "", cb2);

    debug_msg("Sending mod status changed to %s\n", client.c_str());
    list <string> module_names;
    ModuleManager &mmgr(_master_config_tree->module_manager());
    mmgr.get_module_list(module_names);
    list <string>::iterator i;
    for (i = module_names.begin(); i != module_names.end(); i++) {
	debug_msg("module: %s\n", (*i).c_str());
	Module::ModuleStatus status = mmgr.module_status(*i);
	if (status != Module::NO_SUCH_MODULE) {
	    debug_msg("status %d\n", status);
	    _client_interface.send_module_status(client.c_str(),
                 *i, (uint32_t)status, cb2);
	}
    } 
}

XrlCmdError 
XrlRtrmgrInterface::rtrmgr_0_1_enter_config_mode(
	// Input values, 
	const string& token, 
	const bool& exclusive)
{
    string response;

    if (! verify_token(token)) {
	response = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(response);
    }
    uint32_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config") == false) {
	response = "You do not have permission for this operation.";
	return XrlCmdError::COMMAND_FAILED(response);
    }
    XLOG_TRACE(_verbose, "user %d entering config mode\n", user_id);
    if (exclusive)
	XLOG_TRACE(_verbose, "user requested exclusive config\n");
    else
	XLOG_TRACE(_verbose, "user requested non-exclusive config\n");

    //
    // If he's asking for exclusive, and we've already got config users,
    // or there's already an exclusive user, deny the request.
    //
    if (exclusive && !_config_users.empty()) {
	response = "Exclusive config mode requested, but there are already other config mode users\n";
	return XrlCmdError::COMMAND_FAILED(response);
    }
    if (_exclusive && !_config_users.empty()) {
	response = "Another user is in exclusive configuration mode\n";
	return XrlCmdError::COMMAND_FAILED(response);
    }
    multimap<uint32_t, UserInstance*>::iterator iter;
    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	if (iter->second->authtoken() == token) {
	    UserInstance* this_user = iter->second;
	    _config_users.insert(pair<uint32_t, UserInstance*>
				 (user_id, this_user));
	    if (exclusive)
		_exclusive = true;
	    else
		_exclusive = false;
	    return XrlCmdError::OKAY();
	}
    }
    XLOG_UNREACHABLE();
}

XrlCmdError 
XrlRtrmgrInterface::rtrmgr_0_1_leave_config_mode(
	// Input values, 
        const string& token)
{
    if (!verify_token(token)) {
	string err = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    XLOG_TRACE(_verbose, "user %d leaving config mode\n",
	       get_user_id_from_token(token));
    multimap<uint32_t, UserInstance*>::iterator iter;
    for (iter = _config_users.begin(); iter != _config_users.end(); ++iter) {
	if (iter->second->authtoken() == token) {
	    _config_users.erase(iter);
	    return XrlCmdError::OKAY();
	}
    }
    string err = "User was not in config mode.";
    return XrlCmdError::COMMAND_FAILED(err);
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_get_config_users(
	// Input values, 
	const string&	token, 
	// Output values, 
	XrlAtomList&	users)
{
    if (!verify_token(token)) {
	string err = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    uid_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config") == false) {
	string err = "You do not have permission to view the config mode users.";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    multimap<uint32_t, UserInstance*>::const_iterator iter;
    for (iter = _config_users.begin(); iter != _config_users.end(); ++iter) {
	UserInstance* user = iter->second;
	if (!user->is_a_zombie()) {
	    users.append(XrlAtom(user->user_id()));
	} else {
	    XLOG_WARNING("user %d is a zombie\n", user->user_id());
	}
    }
    return XrlCmdError::OKAY();
}

/**
 * This interface is deprecated as the main way for xorpsh to get the
 * running config from the router manager.  It is retained for
 * debugging purposes.  xorpsh now gets its config automatically
 * immediatedly after registering which removes a potential timing
 * hole where the client could be told about a change to the config
 * before it has asked what the current config is.
*/
XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_get_running_config(
	// Input values, 
	const string&	token, 
	// Output values, 
	bool& ready,
	string&	config)
{
    if (!verify_token(token)) {
	string err = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    if (_rtrmgr.ready()) {
	ready = true;
	config = _master_config_tree->show_tree();
    } else {
	ready = false;
	config = "";
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_apply_config_change(
	// Input values, 
	const string&	token, 
	const string&	target, 
	const string&	deltas,
	const string&	deletions)
{
    if (!verify_token(token)) {
	string err = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    if (_config_locked == false) {
	string err = "Cannot commit config changes without locking the config";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    uid_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config") == false) {
	string err = "You do not have permission for this operation.";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    // XXX: TBD
    XLOG_TRACE(_verbose,
	       "\nXRL got config change: deltas: \n%s\nend deltas\ndeletions:\n%s\nend deletions\n",
	       deltas.c_str(), deletions.c_str());

    string response;
    if (_master_config_tree->apply_deltas(user_id, deltas, 
					  /* provisional change */ true,
					  response) == false) {
	return XrlCmdError::COMMAND_FAILED(response);
    }
    if (_master_config_tree->apply_deletions(user_id, deletions, 
					     /* provisional change */ true,
					     response) == false) {
	return XrlCmdError::COMMAND_FAILED(response);
    }
    // Add nodes providing default values.  Note: they shouldn't be
    // needed, but adding them here acts as a safety mechanism against
    // a client that forgets to add them.
    _master_config_tree->add_default_children();

    CallBack cb;
    cb = callback(this, &XrlRtrmgrInterface::apply_config_change_done,
		  user_id, string(target), string(deltas), string(deletions));

    _master_config_tree->commit_changes_pass1(cb);
    if (_master_config_tree->config_failed()) {
	string err = _master_config_tree->config_failed_msg();
	return XrlCmdError::COMMAND_FAILED(err);
    }
    return XrlCmdError::OKAY();
}

void 
XrlRtrmgrInterface::apply_config_change_done(bool success,
					     string errmsg, 
					     uid_t user_id,
					     string target,
					     string deltas,
					     string deletions)
{
    XLOG_TRACE(_verbose,
	       "XRL apply_config_change_done:\n  status:%d\n  response: %s\n  target: %s\n",
	       success, errmsg.c_str(), target.c_str());

    if (success) {
	// Check everything really worked, and finalize the commit
	if (_master_config_tree->check_commit_status(errmsg) == false) {
	    XLOG_TRACE(_verbose,
		       "check commit status indicates failure: >%s<\n",
		       errmsg.c_str());
	    success = false;;
	}
    }  else {
	XLOG_TRACE(_verbose, "request failed: >%s<\n", errmsg.c_str());
    }

    GENERIC_CALLBACK cb1;
    cb1 = callback(this, &XrlRtrmgrInterface::apply_config_change_done_cb);

    if (success) {
	// Send the changes to all the clients, including the client
	// that originally sent them.
	multimap<uint32_t, UserInstance*>::iterator iter;
	for (iter = _users.begin(); iter != _users.end(); ++iter) {
	    string client = iter->second->clientname();
	    GENERIC_CALLBACK cb2;
	    cb2 = callback(this, &XrlRtrmgrInterface::client_updated, 
			   iter->first, iter->second);
	    debug_msg("Sending config changed to %s\n", client.c_str());
	    _client_interface.send_config_changed(client.c_str(),
						  user_id,
						  deltas, deletions, cb2);
	}
	debug_msg("Sending config change done to %s\n", target.c_str());
	_client_interface.send_config_change_done(target.c_str(),
						  true, "", cb1);
    } else {
	// Something went wrong
	_master_config_tree->discard_changes();
	debug_msg("Sending config change failed to %s\n", target.c_str());
	_client_interface.send_config_change_done(target.c_str(),
						  false, errmsg, cb1);
    }
}

void 
XrlRtrmgrInterface::apply_config_change_done_cb(const XrlError& e)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to notify client that config change was done: %s\n",
		   e.error_msg());
    }
}

void 
XrlRtrmgrInterface::client_updated(const XrlError& e, uid_t user_id, 
				   UserInstance* user)
{
    if (e != XrlError::OKAY()) {
	XLOG_ERROR("Failed to notify client that config changed: %s\n",
		   e.error_msg());
	if (e != XrlError::COMMAND_FAILED()) {
	    // We failed to notify the user instance, so we note that
	    // they are probably dead.  We can't guarantee that the
	    // "user" pointer is still valid, so check before using it.
	    multimap<uint32_t, UserInstance*>::iterator iter;
	    for (iter = _users.lower_bound(user_id);
		 iter != _users.end();
		 iter++) {
		if (iter->second == user) {
		    user->set_zombie(true);
		    return;
		}
	    }
	}
    }
}

void 
XrlRtrmgrInterface::module_status_changed(const string& modname,
					  GenericModule::ModuleStatus status)
{
    multimap<uint32_t, UserInstance*>::iterator iter;
    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	string client = iter->second->clientname();
	GENERIC_CALLBACK cb2;
	cb2 = callback(this, &XrlRtrmgrInterface::client_updated, 
		       iter->first, iter->second);
	debug_msg("Sending mod statis changed to %s\n", client.c_str());
	_client_interface.send_module_status(client.c_str(), 
					     modname, (uint32_t)status, cb2);
    }
}


XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_lock_config(
	// Input values, 
	const string&	token, 
	const uint32_t&	timeout /* in milliseconds */, 
	// Output values, 
	bool& success,
	uint32_t& holder)
{

    if (!verify_token(token)) {
	string err = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    uint32_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config") == false) {
	string err = "You do not have permission to lock the configuration.";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    if (_config_locked) {
	// Can't return COMMAND_FAILED and return the lock holder
	success = false;
	holder = get_user_id_from_token(_lock_holder_token);
	return XrlCmdError::OKAY();
    } else {
	success = true;
	_config_locked = true;
	_lock_holder_token = token;
	_lock_timer = _eventloop.new_oneoff_after_ms(
	    timeout, 
	    callback(this, &XrlRtrmgrInterface::lock_timeout));
	return XrlCmdError::OKAY();
    }
}

void XrlRtrmgrInterface::lock_timeout()
{
    _config_locked = false;
    _lock_holder_token = "";
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_unlock_config(
	// Input values
        const string&	token)
{
    if (!verify_token(token)) {
	string err = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    if (_config_locked) {
	if (token != _lock_holder_token) {
	    string err = c_format("Config not held by process %s.", 
				  token.c_str());
	    return XrlCmdError::COMMAND_FAILED(err);
	}
	_config_locked = false;
	_lock_timer.unschedule();
	_lock_holder_token = "";
	return XrlCmdError::OKAY();
    } else {
	string err = "Config not locked.";
	return XrlCmdError::COMMAND_FAILED(err);
    }
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_lock_node(
	// Input values, 
	const string&	token, 
	const string&	node, 
	const uint32_t&	timeout, 
	// Output values, 
	bool& success, 
	uint32_t& holder)
{
    if (!verify_token(token)) {
	string err = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    uint32_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config")==false) {
	string err = "You do not have permission to lock the configuration.";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    if (user_id == (uint32_t)-1) {
	// Shouldn't be possible as we already checked the token
	XLOG_UNREACHABLE();
    }
    success = _master_config_tree->lock_node(node, user_id, timeout, holder);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_unlock_node(
	// Input values, 
	const string&	token, 
	const string&	node)
{
    if (!verify_token(token)) {
	string err = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(err);
    }
    uint32_t user_id = get_user_id_from_token(token);
    if (user_id == (uint32_t)-1) {
	// Shouldn't be possible as we already checked the token
	XLOG_UNREACHABLE();
    }
    bool success;
    success = _master_config_tree->unlock_node(node, user_id);
    if (success) {
	return XrlCmdError::OKAY();
    } else {
	string err = "Unlock failed.";
	return XrlCmdError::COMMAND_FAILED(err);
    }
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_save_config(// Input values: 
					   const string& token, 
					   const string& filename)
{
    string response;

    if (!verify_token(token)) {
	response = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(response);
    }
    uint32_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config")==false) {
	response = "You do not have permission to save the configuration.";
	return XrlCmdError::COMMAND_FAILED(response);
    }
    if (_config_locked) {
	uint32_t uid = get_user_id_from_token(_lock_holder_token);
	string holder = _userdb.find_user_by_user_id(uid)->username();
	response = "ERROR: The config is currently locked by user " +
	    holder + 
	    ", and so may be in an inconsistent state.  \n" + 
	    "Save was NOT performed.\n";
	return XrlCmdError::COMMAND_FAILED(response);
    }
    if (_master_config_tree->save_to_file(filename, user_id, 
					  _rtrmgr.save_hook(), response)) {
	return XrlCmdError::OKAY();
    } else {
	return XrlCmdError::COMMAND_FAILED(response);
    }
}

XrlCmdError
XrlRtrmgrInterface::rtrmgr_0_1_load_config(// Input values:
                                           const string& token,
					   const string& target, 
					   const string& filename)
{
    string response;

    if (!verify_token(token)) {
	response = "AUTH_FAIL";
	return XrlCmdError::COMMAND_FAILED(response);
    }
    uint32_t user_id = get_user_id_from_token(token);
    if (_userdb.has_capability(user_id, "config") == false) {
	response = "You do not have permission to reload the configuration.";
	return XrlCmdError::COMMAND_FAILED(response);
    }
    if (_config_locked && (token != _lock_holder_token)) {
	uint32_t uid = get_user_id_from_token(_lock_holder_token);
	string holder = 
	    _userdb.find_user_by_user_id(uid)->username();
	response = "ERROR: The config is currently locked by user " +
	    holder + 
	    "\n" + 
	    "Load was NOT performed.\n";
	return XrlCmdError::COMMAND_FAILED(response);
    }

    string deltas, deletions;	// These are filled in by load_from_file
    if (_master_config_tree->load_from_file(filename, user_id, response,
					    deltas, deletions)) {
	CallBack cb;
	cb = callback(this, &XrlRtrmgrInterface::apply_config_change_done,
		      user_id, string(target), 
		      string(deltas), string(deletions));
	debug_msg("here1: success\n");
	response = "";
	_master_config_tree->commit_changes_pass1(cb);
	if (_master_config_tree->config_failed()) {
	    string err = _master_config_tree->config_failed_msg();
	    return XrlCmdError::COMMAND_FAILED(err);
	}
    } else {
	return XrlCmdError::COMMAND_FAILED(response);
    }
    return XrlCmdError::OKAY();
}

UserInstance *
XrlRtrmgrInterface::find_user_instance(uint32_t user_id, 
				       const string& clientname)
{
    multimap<uint32_t, UserInstance*>::iterator iter;

    iter = _users.lower_bound(user_id);
    while (iter != _users.end() && iter->second->user_id() == user_id) {
	if (iter->second->clientname() == clientname)
	    return iter->second;
	++iter;
    }
    return NULL;
}

bool
XrlRtrmgrInterface::verify_token(const string& token) const
{
    uint32_t user_id = get_user_id_from_token(token);

    if (user_id == (uint32_t)-1)
	return false;
    multimap<uint32_t, UserInstance*>::const_iterator iter;
    iter = _users.lower_bound(user_id);
    while (iter != _users.end() && iter->second->user_id() == user_id) {
	if (iter->second->authtoken() == token)
	    return true;
	++iter;
    }
    return false;
}

uint32_t
XrlRtrmgrInterface::get_user_id_from_token(const string& token) const
{
    uint32_t user_id;
    size_t tlen = token.size();

    if (token.size() < 10)
	return (uint32_t)-1;
    string uidstr = token.substr(0, 10);
    string authstr = token.substr(10, tlen - 10);
    sscanf(uidstr.c_str(), "%u", &user_id);
    return user_id;
}

string
XrlRtrmgrInterface::generate_auth_token(const uint32_t& user_id, 
					const string& clientname)
{
    string token;
    token = c_format("%10u", user_id);
    string shortcname;
    size_t clen = clientname.size();

    if (clen > CNAMELEN) {
	shortcname = clientname.substr(clen - CNAMELEN, CNAMELEN);
    } else {
	shortcname = clientname;
	for (size_t i = 0; i < (CNAMELEN - clen); i++)
	    shortcname += '*';
    }
    token += shortcname;

    string randomness;
    uint8_t buf[16];
    _randgen.get_random_bytes(16, buf);
    for (size_t i = 0; i < 16; i++)
	randomness += c_format("%2x", buf[i]);
    token += randomness;

    return token;
}


XrlCmdError 
XrlRtrmgrInterface::finder_event_observer_0_1_xrl_target_birth(
        const string&	target_class,
	const string&	target_instance)
{
    XLOG_TRACE(_verbose, "XRL Birth: class %s instance %s\n",
	       target_class.c_str(), target_instance.c_str());
    return XrlCmdError::OKAY();
}


XrlCmdError 
XrlRtrmgrInterface::finder_event_observer_0_1_xrl_target_death(
	const string&	target_class,
	const string&	target_instance)
{
    XLOG_TRACE(_verbose, "XRL Death: class %s instance %s\n",
	       target_class.c_str(), target_instance.c_str());

    multimap<uint32_t, UserInstance*>::iterator iter;
    for (iter = _config_users.begin(); iter != _config_users.end(); ++iter) {
	UserInstance* user = iter->second;
	if (user->clientname() == target_class) {
	    debug_msg("removing user %s from list of config users\n",
		      target_class.c_str());
	    _config_users.erase(iter);
	    break;
	}
    }
    for (iter = _users.begin(); iter != _users.end(); ++iter) {
	UserInstance* user = iter->second;
	if (user->clientname() == target_class) {
	    debug_msg("removing user %s from list of regular users\n",
		      target_class.c_str());
	    _users.erase(iter);
	    break;
	}
    }
    return XrlCmdError::OKAY();
}
