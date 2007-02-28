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

#ident "$XORP: xorp/bgp/attribute_manager.cc,v 1.12 2006/11/08 20:02:48 pavlin Exp $"

//#define DEBUG_LOGGING
#include "bgp_module.h"
#include "attribute_manager.hh"

template <class A>
bool
StoredAttributeList<A>::operator<(const StoredAttributeList<A>& them) const
{
    return (memcmp(hash(), them.hash(), 16) < 0);
}

template <class A>
AttributeManager<A>::AttributeManager()
{
    _total_references = 0;
}

template <class A>
const PathAttributeList<A> *
AttributeManager<A>::add_attribute_list(
    const PathAttributeList<A> *attribute_list)
{
    debug_msg("AttributeManager<A>::add_attribute_list\n");
    StoredAttributeList<A> *new_att =
	new StoredAttributeList<A>(attribute_list);
    typedef typename set<StoredAttributeList<A>*, Att_Ptr_Cmp<A> >::iterator Iter;
    Iter i = _attribute_lists.find(new_att);

    if (i == _attribute_lists.end()) {
	new_att->clone_data();
	_attribute_lists.insert(new_att);
	debug_msg("ATMgr: Inserting new attribute %p\n",
		  new_att->attribute());
	return new_att->attribute();
    }

    (*i)->increase();
    delete new_att;
    debug_msg("** (+) ref count for %p now %d\n",
	      (*i)->attribute(), (*i)->references());
    debug_msg("done\n");

    return (*i)->attribute();
}

template <class A>
void
AttributeManager<A>::delete_attribute_list(
    const PathAttributeList<A> *attribute_list)
{
    StoredAttributeList<A> *del_att =
	new StoredAttributeList<A>(attribute_list);
    debug_msg("AttributeManager<A>::delete_attribute_list %p\n",
	      del_att->attribute());
    typedef typename set<StoredAttributeList<A>*, Att_Ptr_Cmp<A> >::iterator Iter;
    Iter i = _attribute_lists.find(del_att);
    assert(i != _attribute_lists.end());
    delete del_att;
    (*i)->decrease();
    debug_msg("** (-) ref count for %p now %d\n",
	      (*i)->attribute(), (*i)->references());
    if ((*i)->references() == 0) {
	del_att = (*i);
	_attribute_lists.erase(i);
	delete del_att;
    }

}

template class AttributeManager<IPv4>;
template class AttributeManager<IPv6>;









