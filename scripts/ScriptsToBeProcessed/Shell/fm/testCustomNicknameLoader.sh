#! /bin/bash

ls $RANGERHOME/bin/customnicknameload_TestDAILYADDITION2.sh

if [ $? -ne 0 ] 
then
	exit 1
fi

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool testCustomNicknameLoaderV.dat

select 1 from dual where 1 = (select count(*) from custom_nicknames );

select 1 from dual where 1 = (select count(*) from custom_nicknames where nickname = 'Test_Phone Number' and network_id = 999);

select 1 from dual where 1 = (select count(*) from list_configs where name = 'Test_Phone Number');

select 1 from dual where 1 = (select count(*) from scheduler_command_maps );
select 1 from dual where 1 = (select count(*) from scheduler_command_maps where job_name = 'Custom Nickname - Test' and command = 'customnicknameload_TestDAILYADDITION2.sh,TestPid2' and is_network_based = 0);

select 1 from dual where 1 = (select count(*) from list_details where value = 'SELECT value FROM custom_nickname_values WHERE custom_nickname_id = '|| (select id from custom_nicknames  where nickname = 'Test_Phone Number')) ;

select 1 from dual where 1 = (select count(*) from reload_configurations where reload_key = 'PROGRAM_MANAGER.RELOAD.CODE3') ; 
spool off
EOF

if [ $? -ne  '0' ] || grep "no rows" testCustomNicknameLoaderV.dat
then
	exitval=1
else
	exitval=0
fi 

rm -f testCustomNicknameLoaderV.dat
rm -f customnicknameloader.log
exit $exitval

