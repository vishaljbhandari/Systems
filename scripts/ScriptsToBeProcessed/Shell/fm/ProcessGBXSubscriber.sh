#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

cat $WATIR_SERVER_HOME/sitedata/* >> $WATIR_SERVER_HOME/database/subscriber.$$

cp $WATIR_SERVER_HOME/sitedata/* $COMMON_MOUNT_POINT/FMSData/Datasource/Subscriber/
mv $WATIR_SERVER_HOME/sitedata/* $COMMON_MOUNT_POINT/FMSData/Datasource/Subscriber/success

$RANGERHOME/sbin/datasource -n GBXUS_SUBSCRIBERDS > /dev/null 2>&1 &
