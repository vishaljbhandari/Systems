#!/bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


killProcess()
{
echo "Killing process Prevea Core Enginer for this user..."
		
for i in `ps -fu $LOGNAME | grep "prevea" | grep "java" | grep -v "grep" | awk {'print $2'}`
do
	kill -9 $i
done
}

killProcess
