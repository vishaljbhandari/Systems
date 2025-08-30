#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


cat $WATIR_SERVER_HOME/sitedata/* >> $WATIR_SERVER_HOME/database/cdr.$$

cp $WATIR_SERVER_HOME/sitedata/* $COMMON_MOUNT_POINT/FMSData/Datasource/CDR/
mv $WATIR_SERVER_HOME/sitedata/* $COMMON_MOUNT_POINT/FMSData/Datasource/CDR/success

$RANGERHOME/sbin/datasource -n GBXUS_CDRDS > /dev/null 2>&1 &

