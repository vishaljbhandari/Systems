#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $WATIR_SERVER_HOME/Scripts/stopProgrammanager.sh

bash $WATIR_SERVER_HOME/Scripts/CESetup.sh DummyParameterForDBWriter
bash $WATIR_SERVER_HOME/Scripts/generatepartition.sh $1 $2 $3 $4

rm $COMMON_MOUNT_POINT/FMSData/RecordLoaderData/*.*
rm $COMMON_MOUNT_POINT/FMSData/RecordLoaderData/log/*.*

