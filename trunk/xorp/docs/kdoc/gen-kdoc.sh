#!/bin/sh
#
# $XORP: xorp/docs/kdoc/gen-kdoc.sh,v 1.25 2007/03/14 01:33:58 pavlin Exp $
#

#
# Script to generate kdoc documentation for XORP.
#

#
# Defaults
#
DEBUG=0
: ${KDOC_CMD:="kdoc"}

#
# Kdoc format
#
DEFAULT_KDOC_FORMAT="html"
KDOC_FORMAT="${DEFAULT_KDOC_FORMAT}"
KDOC_FORMATS="html check man texinfo docbook latex"

#
# Input and output directories
#
SRC_DIR="../.."
KDOC_DEST_DIR="${SRC_DIR}/docs/kdoc"
KDOC_LIB_DIR="${KDOC_DEST_DIR}/libdir"

#
# Variable for HTML top-level index generation
#
HTML_TEMPLATES="html_templates"
HTML_INDEX="html/index.html"
HTML_INDEX_DATA="html/index.dat"
HTML_CSS="--html-css http://www.xorp.org/xorp-kdoc.css"

#
# Misc. pre-defined variables
#
HTML_LOGO="http://www.xorp.org/images/xorp-logo-medium.jpg"
HTML_LOGO_LINK="http://www.xorp.org/"

#
# Print message to stderr if DEBUG is set to non-zero value
#
dbg_echo()
{
    [ ${DEBUG:-0} -ne 0 ] && echo "$*" >&2
}

#
# glob_files [<glob_pattern1> [<glob pattern2> ...]]
#
# Return glob expanded list of files
# Takes glob patterns as arguments, eg
#  glob_files *.hh *.h
#
glob_files()
{
    FILES=""
    while [ $# -ne 0 ] ; do
	RFILES=`ls ${SRC_DIR}/$1 2>/dev/null`
	if [ -n "${RFILES}" ] ; then
	    FILES="$FILES $RFILES"
	fi
	shift
    done
    echo $FILES
}

#
# exclude files <exclude_glob_pattern> <file_list>
#
# Generates list of exclude files from <exclude_glob_pattern> and filters
# them from <file_list>
#
exclude_files()
{
    if [ $# -eq 1 ] ; then
    	echo "exclude file usage" >&2
    fi
    excluded=`glob_files $1`
    printf "Excluded files:\n  list=\"$excluded\"\n\n" >&2
    shift

    if [ -z "$excluded" ] ; then
	echo "$*"
	return
    fi

    included=""
    for i in $* ; do
	ex=0
	for x in $excluded ; do
	    if [ "$i" = "$x" ] ; then
		ex=1
	    fi
	done
	if [ $ex -eq 0 ] ; then
	    included="$included $i"
	fi
    done
    echo "${included}"
}

#
# Generate xref arguments
#
xref()
{
    XREF=""
    while [ $# -ne 0 ] ; do
	XREF="$XREF --xref $1"
	shift
    done
    echo ${XREF}
}

#
# Run kdoc
# Expects variables "lib", "files", "excludes", "xref" to be set.
#
kdocify() {
    title="Processing lib $lib"
    rule=`echo $title | tr '[A-Za-z0-9_ -]' '='`
    echo "$rule"
    echo "$title"
    echo "$rule"
    printf "Parameters:\n  files=\"$files\" \n  excludes=\"$excludes\"\n  xref=\"$xref\"\n\n"

    excludes=${excludes:-NONE}
    FILES=`glob_files $files`
    FILES=`exclude_files "$excludes" "$FILES"`
    XREFS=`xref $xref`
    OUTDIR=${KDOC_DEST_DIR}/${KDOC_FORMAT}/${lib}
    mkdir -p $OUTDIR
    echo "Created output directory $OUTDIR"
    echo ${FILES} |							      \
    ${KDOC_CMD} --outputdir ${OUTDIR}					      \
	 ${XREFS}         						      \
	 --format ${KDOC_FORMAT}					      \
	 --strip-h-path        	       					      \
	 --name $lib         						      \
	 --libdir ${KDOC_LIB_DIR}					      \
	 --html-logo ${HTML_LOGO}					      \
	 --html-logo-link ${HTML_LOGO_LINK}				      \
	 ${HTML_CSS}							      \
	 --stdin							      \
    | tee ${OUTDIR}/log
    if [ "${KDOC_FORMAT}" = "html" ] ; then
	html_index_add "${lib}" "${desc}" "${html_start_page}"
    fi
}

html_index_start()
{
    mkdir -p html
#
# All the html here is cut and paste from what kdoc generates, it will need
# updating if we move to style sheets or kdoc changes.
#
    cat /dev/null > ${HTML_INDEX_DATA}
}

html_index_end()
{
    local DIR_NAME DESC START_PAGE START_URL WHO WHERE WHEN WITH ROW_BG i

    OIFS="$IFS"
    IFS="@"

    echo "Generating ${HTML_INDEX}"

    cat ${HTML_TEMPLATES}/index.html.top |				      \
        sed -e "s<@HTML_LOGO@<${HTML_LOGO}<" > ${HTML_INDEX}

    i=0
    cat ${HTML_INDEX_DATA} | \
	while read DIR_NAME DESC START_PAGE ; do
	    # Frivalous coloring of cell background to match kdoc
	    if [ `expr $i % 2` -eq 1 ] ; then
		ROW_BG="class=\"leven\""
	    else
		ROW_BG="class=\"lodd\""
	    fi
	    START_URL=${DIR_NAME}/${START_PAGE}
	    cat ${HTML_TEMPLATES}/index.html.entry | sed 		      \
		-e "s<@START_URL@<${START_URL}<"			      \
		-e "s<@DIR_NAME@<${DIR_NAME}<"				      \
		-e "s<@DESC@<${DESC}<"					      \
		-e "s<@ROW_BG@<$ROW_BG<"				      \
		>> ${HTML_INDEX}
	    i=`expr $i + 1`
	done

    WHO=`whoami`
    WHEN=`date`
    WHERE=`hostname`
    WITH=`echo $0 | sed -e 's@.*/@@g'`
    cat ${HTML_TEMPLATES}/index.html.bottom | sed			      \
	-e "s<@WHO@<${WHO}<"						      \
	-e "s<@WHERE@<${WHERE}<"					      \
	-e "s<@WHEN@<${WHEN}<"						      \
	-e "s<@WITH@<${WITH}<"						      \
	>> ${HTML_INDEX}

    # Clean up and restore
    rm -f ${HTML_INDEX_DATA}
    IFS="$OIFS"
}

html_index_add()
{
    echo "$1@$2@$3" >> ${HTML_INDEX_DATA}
}

#
# Print usage string
#
usage()
{
    cat >&2 <<EOF
usage: gen-kdoc.sh [-f <format>]

<format> must be one of "${KDOC_FORMATS}"
  default="${DEFAULT_KDOC_FORMAT}"

Note: option -f can be applied once.

EOF

    exit 1
}

validate_format()
{
    local FMT
    for FMT in ${KDOC_FORMATS} ; do
	if [ "X${FMT}" = "X${KDOC_FORMAT}" ] ; then
	    return
	fi
    done

    usage
}

#
# Main
#
while getopts "f:h?" ch
do
    case $ch in
        f) KDOC_FORMAT=$OPTARG ;;
        *) usage ;;
    esac
done

validate_format

if [ "${KDOC_FORMAT}" = "html" ] ; then
    html_index_start
fi

###############################################################################
#
# Most of the work of this script is done by the kdocify routine.  kdocify
# relies on the existence of a number of global variables to run correctly.
#
# The following MUST  be set before calling kdocify:
#
# lib		  := directory name as it appears under the top level
#		     of the xorp tree.
#
# desc 		  := textual description for the top-level html documentation.
#
# html_start_page := html start page under the html/${lib}/ directory.  For
#		     pure C code, the kdoc index.html file is not useful
#		     since it normally lists classes and structures and not
#		     globally defined functions.
#
# files		  := files to parsed by kdoc under the top level of the xorp
#		     tree.
#
# excludes	  := files to excluded from from parsing.  Exists since
#		     it's easier to wildcard include files, then exclude
#		     a few.
#
# xref		  := list of already kdoc'ed directories to cross reference
#		     against.
#
###############################################################################

#
# libxorp
#
kdoc_libxorp()
{
    lib="libxorp"
    desc="XORP core type and utility library"
    html_start_page="index.html"
    files="libxorp/*.h libxorp/*.hh"
    excludes="libxorp/callback.hh libxorp/callback_debug.hh libxorp/callback_nodebug.hh libxorp/old_trie.hh"
    xref=""
    kdocify
}

#
# callback.hh in libxorp
#
kdoc_callback()
{
    lib="libxorp-callback"
    desc="XORP callback routines"
    html_start_page="index.html"
    files="libxorp/callback_nodebug.hh"
    excludes=""
    xref="libxorp"
    kdocify
}

#
# libcomm
#
kdoc_libcomm()
{
    lib="libcomm"
    desc="Socket library"
    html_start_page="all-globals.html"
    files="libcomm/*.h libcomm/*.hh"
    excludes=""
    xref="libxorp libxorp-callback"
    kdocify
}

#
# libxipc
#
kdoc_libxipc()
{
    lib="libxipc"
    desc="XORP interprocess communication library"
    html_start_page="index.html"
    files="libxipc/*.h libxipc/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm"
    kdocify
}

#
# xrl_interfaces
#
kdoc_xrl_interfaces()
{
    lib="xrl-interfaces"
    desc="Generated code for sending Xrl's to Targets"
    html_start_page="index.html"
    files="xrl/interfaces/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc"
    kdocify
}

#
# xrl_targets
#
kdoc_xrl_targets()
{
    lib="xrl-targets"
    desc="Generated Xrl Target base classes"
    html_start_page="index.html"
    files="xrl/targets/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc"
    kdocify
}

#
# libproto
#
kdoc_libproto()
{
    lib="libproto"
    desc="Protocol Node library used by XORP multicast processes"
    html_start_page="index.html"
    files="libproto/*.h libproto/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc"
    kdocify
}

#
# mrt
#
kdoc_mrt()
{
    lib="mrt"
    desc="Multicast Routing Table library"
    html_start_page="index.html"
    files="mrt/*.h mrt/*.hh"
    excludes=""
    xref="libxorp"
    kdocify
}

#
# cli
#
kdoc_cli()
{
    lib="cli"
    desc="Command Line Interface library"
    html_start_page="index.html"
    files="cli/*.h cli/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets libproto"
    kdocify
}

#
# libfeaclient
#
kdoc_libfeaclient()
{
    lib="libfeaclient"
    desc="Forwarind Engine Abstraction Client library"
    html_start_page="index.html"
    files="libfeaclient/*.h libfeaclient/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets"
    kdocify
}

#
# fea
#
kdoc_fea()
{
    lib="fea"
    desc="Forwarding Engine Abstraction daemon"
    html_start_page="index.html"
    files="fea/*.h fea/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets libproto mrt cli libfeaclient"
    kdocify
}

#
# mld6igmp
#
kdoc_mld6igmp()
{
    lib="mld6igmp"
    desc="Multicast Listener Discovery daemon"
    html_start_page="index.html"
    files="mld6igmp/*.h mld6igmp/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc libproto mrt cli fea xrl-interfaces xrl-targets libfeaclient"
    kdocify
}

#
# pim
#
kdoc_pim()
{
    lib="pim"
    desc="Protocol Independent Multicast (PIM) daemon"
    html_start_page="index.html"
    files="pim/*.h pim/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets libfeaclient libproto mrt cli fea mld6igmp"
    kdocify
}

#
# Policy manager
# 
kdoc_policy()
{
    lib="policy"
    desc="Policy manager daemon"
    html_start_page="index.html"
    files="policy/*.h policy/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets libproto policy-common"
    kdocify
}

#
# Policy backend
# 
kdoc_libpolicybackend()
{
    lib="libpolicybackend"
    desc="Policy backend filter"
    html_start_page="index.html"
    files="policy/backend/*.h policy/backend/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets libproto policy-common"
    kdocify
}

#
# Policy common
# 
kdoc_policycommon()
{
    lib="policy-common"
    desc="Policy shared routines between backend/frontend"
    html_start_page="index.html"
    files="policy/common/*.h policy/common/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets libproto"
    kdocify
}

#
# bgp
#
kdoc_bgp()
{
    lib="bgp"
    desc="BGP4 daemon"
    html_start_page="index.html"
    files="bgp/*.h bgp/*.hh"
    excludes="bgp/test_*.h bgp/test_*.hh"
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets libfeaclient libpolicybackend"
    kdocify
}

#
# fib2mrib
#
kdoc_fib2mrib()
{
    lib="fib2mrib"
    desc="FIB2MRIB daemon"
    html_start_page="index.html"
    files="fib2mrib/*.h fib2mrib/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets libfeaclient libpolicybackend"
    kdocify
}

#
# mibs
#
kdoc_mibs()
{
    lib="mibs"
    desc="MIB modules for Net-SNMP"
    html_start_page="index.html"
    files="mibs/*.h mibs/*.hh"
    excludes="mibs/patched_container.h"
    xref="libxorp libxipc xrl-interfaces xrl-targets"
    kdocify
}

#
# ospf
#
kdoc_ospf()
{
    lib="ospf"
    desc="Open Shortest Path First daemon"
    html_start_page="index.html"
    files="ospf/*.h ospf/*.hh"
    excludes="ospf/test_*.h ospf/test_*.hh"
    xref="libxorp libxorp-callback libxipc xrl-interfaces xrl-targets libproto libfeaclient libpolicybackend"
    kdocify
}

#
# rib
#
kdoc_rib()
{
    lib="rib"
    desc="Routing Information Base daemon"
    html_start_page="index.html"
    files="rib/*.h rib/*.hh"
    excludes="rib/dummy_register_server.hh rib/parser_direct_cmds.hh rib/parser_xrl_cmds.hh rib/parser.hh"
    xref="libxorp libxorp-callback xrl-interfaces xrl-targets libproto libfeaclient libpolicybackend"
    kdocify
}

#
# rip
#
kdoc_rip()
{
    lib="rip"
    desc="Routing Information Protocol"
    html_start_page="index.html"
    files="rip/*.h rip/*.hh"
    excludes="rip/test_*.h rip/test_*.hh"
    xref="libxorp libxorp-callback libxipc xrl-interfaces xrl-targets libfeaclient libpolicybackend"
    kdocify
}

#
# rtrmgr
#
kdoc_rtrmgr()
{
    lib="rtrmgr"
    desc="Router Manager"
    html_start_page="index.html"
    files="rtrmgr/*.h rtrmgr/*.hh"
    excludes="rtrmgr/test_*.h rtrmgr/test_*.hh"
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets"
    kdocify
}

#
# static_routes
#
kdoc_static_routes()
{
    lib="static_routes"
    desc="Static Routes daemon"
    html_start_page="index.html"
    files="static_routes/*.h static_routes/*.hh"
    excludes=""
    xref="libxorp libxorp-callback libcomm libxipc xrl-interfaces xrl-targets libfeaclient libpolicybackend"
    kdocify
}


KDOC_ALL_TGTS="libxorp callback libcomm libxipc libproto xrl_interfaces \
	       xrl_targets mrt cli libfeaclient fea mld6igmp pim policycommon \
	       libpolicybackend policy bgp fib2mrib mibs ospf rib rip rtrmgr static_routes"
: ${KDOC_TGTS:=${KDOC_ALL_TGTS}}
for i in ${KDOC_TGTS} ; do
    kdoc_$i
done

#
# Build html index if appropriate
#
if [ "${KDOC_FORMAT}" = "html" ] ; then
    html_index_end
fi
