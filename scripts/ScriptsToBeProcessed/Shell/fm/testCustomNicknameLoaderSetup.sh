#! /bin/bash
cp ./Tests/testDataForCustomNicknameLoading.dat $COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/Test_1025_2/new/Test1025.txt 

rm $RANGERHOME/bin/customnicknameload_TestDAILYADDITION.sh 

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;

delete from custom_nickname_values ;
delete from custom_nicknames ;
delete from list_details ;
delete from list_configs_networks ;
delete from list_configs ;
delete from scheduler_command_maps ;
delete from reload_configurations ;

commit;
quit;
EOF
if test $? -eq 5
then
	exit 1
fi

exit 0
	

