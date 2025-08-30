#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

dir_name=$1
echo $dir_name

DAY=`/bin/date +%Y%m%d`

touch  $COMMON_MOUNT_POINT/InMemory/FingerPrint/$dir_name/delta/"cache_00000_${DAY}_235959"
touch  $COMMON_MOUNT_POINT/InMemory/FingerPrint/$dir_name/delta/"cache_00001_${DAY}_235959"
touch  $COMMON_MOUNT_POINT/InMemory/FingerPrint/$dir_name/delta/"cache_00002_${DAY}_235959"
touch  $COMMON_MOUNT_POINT/InMemory/FingerPrint/$dir_name/delta/"cache_00003_${DAY}_235959"

entity_name=($1)

ruby $RANGERHOME/rbin/generatefingerprintprofilefor$entity_name.rb