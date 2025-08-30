#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $WATIR_SERVER_HOME/Scripts/ClearRecords.sh >/dev/null 2>&1

if [ ! `ps -u$LOGNAME | grep programmanager | grep -v "grep programmanager" | awk {'print $1'}` ]
then
        rm -f /tmp/.*FIFO*
        $RANGERHOME/sbin/programmanager -f $RANGERHOME/sbin/programmanager.conf.tcp  < /dev/null >& /dev/null &
fi