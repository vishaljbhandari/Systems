#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

if [ -f $RANGERHOME/rbin/$1 ]
then
	echo "running $1 from rbin"
	ruby $RANGERHOME/rbin/$*

elif [ -f $RANGERHOME/bin/$1 ]
then
 	echo "running $1 from bin"
	scriptlauncher $RANGERHOME/bin/$*
else
	echo "running $1 from sbin"
	$RANGERHOME/sbin/$*
fi
