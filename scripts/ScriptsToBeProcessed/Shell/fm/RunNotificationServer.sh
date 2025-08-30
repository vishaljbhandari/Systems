#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

if [ ! `ps -fu$LOGNAME | grep "server.rb" | grep -v "grep server.rb" | awk {'print $2'}` ]
then
    cd $RANGERV6_NOTIFICATIONMANAGER_HOME
    ./server.rb  < /dev/null >& /dev/null &
fi

#Starting server for WSDL
if [ ! `ps -fu$LOGNAME | grep "BillingInfoService.rb" | grep -v "grep BillingInfoService.rb" | awk {'print $2'}` ]
then
    cd $RANGERV6_NOTIFICATIONMANAGER_HOME
    cd ../BillingInfoServiceServer/
    ruby BillingInfoService.rb < /dev/null >& /dev/null &
fi

