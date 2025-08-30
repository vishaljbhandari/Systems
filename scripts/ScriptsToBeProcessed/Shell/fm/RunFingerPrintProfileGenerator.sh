#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

dir_name=$1
echo $dir_name
scriptname=profile_manager.rb
DAY=`/bin/date +%Y%m%d`


if [ $# -eq 0 ]
then 
#echo "running $scriptname for all entities"
bash $WATIR_SERVER_HOME/Scripts/update_profile_manager_entity_configs.sh $dir_name
ruby $RANGERHOME/rbin/$scriptname
else
	touch  $COMMON_MOUNT_POINT/InMemory/ProfileElements/$dir_name/delta/"cache_00000_${DAY}_235959"
	touch  $COMMON_MOUNT_POINT/InMemory/ProfileElements/$dir_name/delta/"cache_00001_${DAY}_235959"
	touch  $COMMON_MOUNT_POINT/InMemory/ProfileElements/$dir_name/delta/"cache_00002_${DAY}_235959"
	touch  $COMMON_MOUNT_POINT/InMemory/ProfileElements/$dir_name/delta/"cache_00003_${DAY}_235959"
	touch  $COMMON_MOUNT_POINT/InMemory/ProfileElements/$dir_name/delta/"cache_00004_${DAY}_235959"
	touch  $COMMON_MOUNT_POINT/InMemory/ProfileElements/$dir_name/delta/"cache_00005_${DAY}_235959"
	touch  $COMMON_MOUNT_POINT/InMemory/ProfileElements/$dir_name/delta/"cache_00006_${DAY}_235959"
	touch  $COMMON_MOUNT_POINT/InMemory/ProfileElements/$dir_name/delta/"cache_00007_${DAY}_235959"

	bash $WATIR_SERVER_HOME/Scripts/update_profile_manager_entity_configs.sh $dir_name

		
		if [ "$dir_name" == "InternalUser" ]
		then
		dir_name="Internal User"
		echo $dir_name
		elif [ "$dir_name" == "OtherPartyNumber" ]
		then
		dir_name="OtherParty Number"	
		echo $dir_name
		fi

	id=`$WATIR_SERVER_HOME/Scripts/getEntityID.sh "FP_ENTITIES" "$dir_name"`
	id=`echo $id | sed 's/ //g'`
echo "running $scriptname for entity $id"
ruby $RANGERHOME/rbin/$scriptname $id 0
fi
