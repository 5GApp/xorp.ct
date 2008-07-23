// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/common/policy_utils.cc,v 1.13 2008/01/04 03:17:19 pavlin Exp $"

#include "libxorp/xorp.h"

#if defined(HAVE_REGEX_H)
#include <regex.h>
#elif defined(HAVE_PCREPOSIX_H)
#include <pcre.h>
#include <pcreposix.h>
#else
#error "No header defining regex_t is present."
#endif

#include "policy_utils.hh"


namespace policy_utils {

void 
str_to_list(const string& in, list<string>& out)
{
    string::size_type pos1 = 0;	// beginning of token
    string::size_type pos2 = 0;  // end of token
    string::size_type len = in.length();
    string token;

    while(pos1 < len) {

	// find delimiter
        pos2 = in.find(",",pos1);

	// none found, so treat end of string as delim
        if(pos2 == string::npos) {
                token = in.substr(pos1,len-pos1);

                out.push_back(token);
                return;
        }
	
	// grab token [delimiter found].
        token = in.substr(pos1,pos2-pos1);
        out.push_back(token);
        pos1 = pos2+1;
    }
}

void 
str_to_set(const string& in, set<string>& out)
{
    list<string> tmp;

    // XXX: slow
    str_to_list(in,tmp);

    for(list<string>::iterator i = tmp.begin();
	i != tmp.end(); ++i)

	// since it is a set, dups will be removed.
	out.insert(*i);
}

void
read_file(const string& fname, string& out)
{
    char buff[4096];
    int rd;

    string err;

    // open file
    FILE* f = fopen(fname.c_str(),"r");
    if(!f) {
        err += "Unable to open file " + fname + ": ";
        err += strerror(errno);

        xorp_throw(PolicyUtilsErr, err);
    }

    buff[0] = 0;

    // read file
    while(!feof(f)) {
        rd = fread(buff,1,sizeof(buff)-1,f);
        if(rd == 0)
            break;
        if(rd < 0) {
            err += "Unable to read file " + fname + ": ";
            err += strerror(errno);

            fclose(f);
            xorp_throw(PolicyUtilsErr, err);
        }

	// append to content of file to out
        buff[rd] = 0;
        out += buff;
    }

    fclose(f);
    return;
}

unsigned 
count_nl(const char* x)
{
    const char* end = &x[strlen(x)];
    unsigned nl = 0;

    for(const char* ptr = x; ptr < end; ptr++)
	if(*ptr == '\n')
	    nl++;

    return nl;
}

bool
regex(const string& str, const string& reg)
{
    // compile the regex
    regex_t re;
    int res = regcomp(&re, reg.c_str(), REG_EXTENDED);
    
    if (res) {
	char tmp[128];
	string err;

	regerror(res, &re, tmp, sizeof(tmp));
	regfree(&re);

	err = "Unable to compile regex (" + reg;
	err += "): ";
	err += tmp;

	xorp_throw(PolicyUtilsErr, err);
    }

    // execute the regex [XXX: check for errors!!]
    bool result = !regexec(&re, str.c_str(), 0, 0, 0);
    regfree(&re);

    return result;
}

} // namespace
