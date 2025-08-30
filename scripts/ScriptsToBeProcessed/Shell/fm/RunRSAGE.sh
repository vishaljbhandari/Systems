#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


if [ $1 == "Subscriber" ]
then
 
$RANGERHOME/sbin/recorddispatcher -r"DB://SUBSCRIBER" -c"SUBSCRIBERGROUPING" -i"SUBINST" -e "SUBSCRIBER_TYPE =0 AND STATUS in(1,2)"< /dev/null >& /dev/null &
echo "Running RSAGE for Subscriber"

else

$RANGERHOME/sbin/recorddispatcher -r"DB://4" -c"ACCOUNTGROUPING" -i"ACCINST" -e "ACCOUNT_TYPE=0"< /dev/null >& /dev/null &
echo "Running RSAGE for Account"

fi
