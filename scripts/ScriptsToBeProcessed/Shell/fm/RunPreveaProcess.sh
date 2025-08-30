#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

if [ ! `ps -fu$LOGNAME | grep "prevea" | grep "java" | grep -v "grep" | awk {'print $1'}` ]
then
	cd $PREVEA_HOME/sbin
    ./run-prevea.sh < /dev/null >& /dev/null &
fi
