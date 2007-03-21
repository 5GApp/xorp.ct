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

// $XORP: xorp/rtrmgr/userdb.hh,v 1.12 2006/04/26 04:42:31 pavlin Exp $

#ifndef __RTRMGR_USERDB_HH__
#define __RTRMGR_USERDB_HH__


#include <map>
#include <set>


class User {
public:
    User(uid_t user_id, const string& username);

    const string& username() const { return _username; }
    uid_t user_id() const { return _user_id; }
    bool has_acl_capability(const string& capname) const;
    void add_acl_capability(const string& capname);

private:
    uid_t	_user_id;
    string	_username;
    set<string> _capabilities;
};

//
// The same user may be logged in multiple times, so logged in users
// get a UserInstance.
//
class UserInstance : public User {
public:
    UserInstance(uid_t user_id, const string& username);

    const string& clientname() const { return _clientname; }
    void set_clientname(const string& clientname) { _clientname = clientname; }

    uint32_t clientid() const { return _clientid; }
    void set_clientid(uint32_t clientid) { _clientid = clientid; }

    const string& authtoken() const { return _authtoken; }
    void set_authtoken(const string& authtoken) { _authtoken = authtoken; }

    bool is_authenticated() const { return _authenticated; }
    void set_authenticated(bool authenticated) {
	_authenticated = authenticated;
    }

    bool is_in_config_mode() const { return _config_mode; }
    void set_config_mode(bool is_in_config_mode) {
	_config_mode = is_in_config_mode;
    }

    bool is_a_zombie() const { return _is_a_zombie; }
    void set_zombie(bool state) { _is_a_zombie = state; }

private:
    string _clientname;
    uint32_t	_clientid;  /* client ID is a unique number for every
			       connected client at any moment in time,
			       but not guaranteed to be unique over
			       time */
    string _authtoken;
    bool _authenticated;
    bool _config_mode;
    bool _is_a_zombie;	// A user instance is a zombie if we suspect
			// the client process no longer exists
};

class UserDB {
public:
    UserDB(bool verbose);
    ~UserDB();

    User* add_user(uid_t user_id, const string& username, gid_t gid);
    void load_password_file();
    const User* find_user_by_user_id(uid_t user_id);
    void remove_user(uid_t user_id);
    bool has_capability(uid_t user_id, const string& capability);

private:
    map<uid_t, User*> _users;
    bool _verbose;	// Set to true if output is verbose
};

#endif // __RTRMGR_USERDB_HH__
