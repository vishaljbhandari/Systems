#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

if [ `ps -fu $LOGNAME | grep "inline_request_processor.rb" | grep -v "grep inline_request_processor.rb" | awk {'print $2'}` ]
then
	for i in `ps -fu $LOGNAME | grep "inline_request_processor.rb" | grep -v "grep" | awk {'print $2'}`
        do
               	kill -9 $i
        done
fi
sleep 5
cd $RANGERV6_CLIENT_HOME/../InlineWrapperModule/script
nohup ruby inline_request_processor.rb $COMMON_MOUNT_POINT/FMSData/InlineDataRecord/Process 1 < /dev/null >& /dev/null &

