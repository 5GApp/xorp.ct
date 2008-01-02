#!/bin/sh

# $XORP: other/tinderbox/scripts/tinderbox.sh,v 1.17 2007/08/03 22:21:26 pavlin Exp $

CONFIG="$(dirname $0)/config"
. ${CONFIG}

filter_and_post_log()
{
    local subject toaddr errfile filtfile msgfile

    d=$(date "+%y%m%d-%H%M")
    subject="[$d] - $1"
    toaddr="$2"
    errfile="$3"

    # File containing filtered error log
    filtfile=${errfile}.filt

    # File containing email message
    msgfile=${errfile}.mail.msg

    cat ${errfile} | awk -f ${SCRIPTSDIR}/filter-error.awk > ${filtfile}

    cat ${SCRIPTSDIR}/boilerplate ${filtfile} | 			      \
      sed -e "s/@To@/${toaddr}/"					      \
	  -e "s/@From@/${FROMADDR}/"					      \
	  -e "s/@ReplyTo@/${REPLYTOADDR}/"				      \
	  -e "s/@Subject@/${subject}/"					      \
      > ${msgfile}
    ${POST} ${msgfile}

#    rm -f ${filtfile} ${msgfile}
}

#
# init_log_header
#
# arguments should be obvious...
#
init_log_header()
{
    local outfile cfg host env dir now sinfo 
    outfile="$1"
    cfg="$2"
    host="$3"
    env="$4"
    dir="$5"
    ssh_flags="$6"
    now=$(date "+%Y-%m-%d %H:%M:%S %Z")
    sinfo=$(ssh ${ssh_flags} -n ${host} 'uname -s -r')
    cat >${outfile} <<EOF
-------------------------------------------------------------------------------
Configuration:    ${cfg}
Host:             ${host}
SSH flags:        ${ssh_flags}
Environment:      ${env}
Build directory:  ${dir}
Start time:       ${now}
System Info:      ${sinfo}
-------------------------------------------------------------------------------

EOF
}

#
# extoll - send success message
#
# $1 = Config message relevant to + message
# $2 = log file
#
extoll()
{
    filter_and_post_log "PASS: $1" "${REPORTOKADDR}" "${2}"
}

#
# harp - send fail message
#
# $1 = Message relevant to
# $2 = log file
#
harp()
{
    filter_and_post_log "FAIL: $1" "${REPORTBADADDR}" "${2}"
}

run_tinderbox() {
    #
    # Copy and build latest code on tinderbox machines
    #
    for cfg in ${TINDERBOX_CONFIGS} ; do
	eval cfg_host=\$host_$cfg
	eval cfg_home=\$home_$cfg
	eval cfg_env=\$env_$cfg
	eval cfg_buildflags=\$buildflags_$cfg
	eval cfg_sshflags=\$sshflags_$cfg
	eval cfg_disable_check=\${disable_check}_$cfg

	# Add the common environment
	cfg_env="${cfg_env} ${COMMON_ENV}"

	errfile="${LOGDIR}/0/${cfg_host}-${cfg}"
	header="${errfile}.header"

	init_log_header "${header}" "${cfg}" "${cfg_host}" "${cfg_env}" "${cfg_home}" "${cfg_sshflags}"
	cp ${header} ${errfile}
 
	if [ -z "${cfg_host}" ] ; then
	    echo "FAIL: Configuration \"$cfg\" has no host." >> ${errfile}
	    harp "${cfg} configuration" "${errfile}"
	    continue
	fi

	if [ -z "${cfg_home}" ] ; then
	    echo "FAIL: Configuration \"$cfg\" has no directory." >> ${errfile}
	    harp "${cfg} configuration" "${err_file}"
	    continue
	fi

	echo "Remote copy ${cfg_host} ${cfg_home} ${cfg_sshflags}" >> ${errfile}
	${SCRIPTSDIR}/remote_xorp_copy.sh "${cfg_host}" "${cfg_home}" "${cfg_sshflags}" >> ${errfile} 2>&1
	if [ $? -ne 0 ] ; then
	    harp "${cfg} remote copy" "${errfile}"
	    continue
	fi

	# Log in to host and build xorp
	build_errfile="${errfile}-build"
	cp ${header} ${build_errfile}
	ssh ${cfg_sshflags} -n ${cfg_host} "env ${cfg_env} ${cfg_home}/scripts/build_xorp.sh ${cfg_buildflags}" >>${build_errfile} 2>&1
	if [ $? -ne 0 ] ; then
	    harp "${cfg} remote build" "${build_errfile}"
	    continue
	fi

	# Kill tinderbox user processes that may be lying around from
	# earlier runs (boo, hiss)
	ssh ${cfg_sshflags} -n ${cfg_host}	"kill -- -1" 2>/dev/null

	# Log into host and run regression tests
	if [ "x${cfg_disable_check}" != "xyes" ] ; then
	    check_errfile="${errfile}-check"
	    cp ${header} ${check_errfile}
	    ssh ${cfg_sshflags} -n ${cfg_host} "env ${cfg_env} ${cfg_home}/scripts/build_xorp.sh check" >>${check_errfile} 2>&1
	    if [ $? -ne 0 ] ; then
		harp "${cfg} remote check" "${check_errfile}"
		continue
	    fi
	fi

	cp ${header} ${errfile}
	echo "No problems to report." >> ${errfile}
	extoll "${cfg}" "${errfile}"
    done
}

checkout() {
    local errfile
    errfile="$1"

    cat /dev/null > ${errfile}

    #
    # Checkout latest code
    #
    ${SCRIPTSDIR}/co_xorp.sh >${errfile} 2>&1
    if [ $? -ne 0 ] ; then
	harp "$LOCALHOST checkout" "${errfile}"
	exit
    fi
}

roll_over_logs()
{
    log_history=15
    rm -rf ${LOGDIR}/${log_history}
    log=${log_history}
    while [ ${log} -ne 0 ] ; do
	next_log=$((${log} - 1))
	if [ -d ${LOGDIR}/${next_log} ] ; then
	    mv ${LOGDIR}/${next_log} ${LOGDIR}/${log}
	fi
	log=${next_log}
    done
}

#
# Implement some simple locking to make sure we don't run two
# instances of tinderbox.sh from cron at the same time.
#
tinderbox_lockfile="${LOGDIR}/tinderbox.lock"
if [ -f "${tinderbox_lockfile}" ]; then
	echo "Tinderbox was invoked whilst already running."
	exit 255
fi

touch ${tinderbox_lockfile}

roll_over_logs
mkdir -p ${LOGDIR}/0

checkout "${LOGDIR}/0/checkout"
run_tinderbox

rm -f ${tinderbox_lockfile}
