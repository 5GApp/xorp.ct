#!/bin/sh

#
# $XORP: xorp/bgp/harness/xrl_shell_funcs.sh,v 1.8 2004/06/04 23:58:36 atanu Exp $
#

CALLXRL=${CALLXRL:-../../libxipc/call_xrl -w 10}
BASE=${BASE:-test_peer} # Set BASE in callers environment.

#
# Command to the coordinator.
#
coord()
{
    echo "Coord $* "
    $CALLXRL "finder://coord/coord/0.1/command?command:txt=$*"

    if [ "${NOBLOCK:-false}" = "true" ]
    then
	return
    fi

    # Only try five times for the operation to complete.
    local i
    for i in 1 2 3 4 5
    do
	if ! $CALLXRL "finder://coord/coord/0.1/pending" |grep true > /dev/null
	then
	    return
	fi
	echo "Operation in coordinator still pending try number: $i"
	sleep 1
    done

    return -1
}

status()
{
#    echo -n "Status $* "
    $CALLXRL "finder://coord/coord/0.1/status?peer:txt=$1" |
    sed -e 's/^status:txt=//' -e 's/+/ /g'
}

pending()
{
    echo -n "Pending "
    $CALLXRL "finder://coord/coord/0.1/pending"
}

#
# Commands to the test peers
#
register()
{
    echo -n "Register $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/register?coordinator:txt=$1"
}

packetisation()
{
    echo -n "Packetisation $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/packetisation?protocol:txt=$1"
}

connect()
{
    echo -n "Connect $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/connect?host:txt=$1&port:u32=$2"
}

send()
{
    echo -n "Send $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/send?data:txt=$*"
}

listen()
{
    echo -n "Listen $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/listen?address:txt=$1&port:u32=$2"
}

bind()
{
    echo -n "Bind $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/bind?address:txt=$1&port:u32=$2"
}

disconnect()
{
    echo -n "Disconnect $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/disconnect"
}

reset()
{
    echo -n "Reset $* "
    $CALLXRL "finder://$BASE/test_peer/0.1/reset"
}

terminate()
{
    echo -n "Terminate "
    $CALLXRL "finder://$BASE/test_peer/0.1/terminate"
}

# We have arguments.
if [ $# != 0 ]
then
    $*
fi

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
