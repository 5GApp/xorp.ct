/*
 * Note: this file originally auto-generated by mib2c using
 *        : mib2c.array-user.conf,v 5.15.2.1 2003/02/27 05:59:41 rstory Exp $
 *
 * $Id$
 *
 * Yes, there is lots of code here that you might not use. But it is much
 * easier to remove code than to add it!
 */
#ifndef BGP4PATHATTRTABLE_H
#define BGP4PATHATTRTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

    
#include <net-snmp/net-snmp-config.h>

#ifdef HAVE_NET_SNMP_LIBRARY_CONTAINER_H
#include <net-snmp/library/container.h>
#else
#include "patched_container.h"
#endif

#include <net-snmp/agent/table_array.h>

        /** Index bgp4PathAttrIpAddrPrefix is internal */
        /** Index bgp4PathAttrIpAddrPrefixLen is internal */
        /** Index bgp4PathAttrPeer is internal */

typedef struct bgp4PathAttrTable_context_s {
    netsnmp_index index; // THIS MUST BE FIRST!!! /

    unsigned long bgp4PathAttrPeer;
    unsigned long bgp4PathAttrIpAddrPrefixLen;
    unsigned long bgp4PathAttrIpAddrPrefix;
    long bgp4PathAttrOrigin;
    vector<uint8_t> bgp4PathAttrASPathSegment;
    unsigned long bgp4PathAttrNextHop;
    long bgp4PathAttrMultiExitDisc;
    long bgp4PathAttrLocalPref;
    long bgp4PathAttrAtomicAggregate;
    long bgp4PathAttrAggregatorAS;
    unsigned long bgp4PathAttrAggregatorAddr;
    long bgp4PathAttrCalcLocalPref;
    long bgp4PathAttrBest;
    vector<uint8_t> bgp4PathAttrUnknown;

    uint32_t update_signature;
} bgp4PathAttrTable_context;

/*************************************************************
 * function declarations
 */
void init_bgp4_mib_1657_bgp4pathattrtable(void);
void deinit_bgp4_mib_1657_bgp4pathattrtable(void);
void initialize_table_bgp4PathAttrTable(void);
const bgp4PathAttrTable_context * bgp4PathAttrTable_get_by_idx(netsnmp_index *);
const bgp4PathAttrTable_context * bgp4PathAttrTable_get_by_idx_rs(netsnmp_index *,
                                        int row_status);
int bgp4PathAttrTable_get_value(netsnmp_request_info *, netsnmp_index *, netsnmp_table_request_info *);


/*************************************************************
 * oid declarations
 */
extern oid bgp4PathAttrTable_oid[];
extern size_t bgp4PathAttrTable_oid_len;

#define bgp4PathAttrTable_TABLE_OID 1,3,6,1,2,1,15,6
    
/*************************************************************
 * column number definitions for table bgp4PathAttrTable
 */
#define COLUMN_BGP4PATHATTRPEER 1
#define COLUMN_BGP4PATHATTRIPADDRPREFIXLEN 2
#define COLUMN_BGP4PATHATTRIPADDRPREFIX 3
#define COLUMN_BGP4PATHATTRORIGIN 4
#define COLUMN_BGP4PATHATTRASPATHSEGMENT 5
#define COLUMN_BGP4PATHATTRNEXTHOP 6
#define COLUMN_BGP4PATHATTRMULTIEXITDISC 7
#define COLUMN_BGP4PATHATTRLOCALPREF 8
#define COLUMN_BGP4PATHATTRATOMICAGGREGATE 9
#define COLUMN_BGP4PATHATTRAGGREGATORAS 10
#define COLUMN_BGP4PATHATTRAGGREGATORADDR 11
#define COLUMN_BGP4PATHATTRCALCLOCALPREF 12
#define COLUMN_BGP4PATHATTRBEST 13
#define COLUMN_BGP4PATHATTRUNKNOWN 14
#define bgp4PathAttrTable_COL_MIN 1
#define bgp4PathAttrTable_COL_MAX 14

#define UPDATE_REST_INTERVAL_ms 5000

#ifdef __cplusplus
};
#endif

#endif /** BGP4PATHATTRTABLE_H */
