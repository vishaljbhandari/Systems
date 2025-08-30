#! /bin/bash

ls $RANGERHOME/bin/customnicknameload_TestDAILYADDITION2.sh

if [ $? -ne 0 ] 
then
	echo "At exit 1"
	exit 1
fi

cmp generated_customnicknamefile.txt $RANGERHOME/bin/customnicknameload_TestDAILYADDITION2.sh

if [ $? -ne 0 ] 
then
	echo "At exit 2"
	exit 2
fi

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool testCustomNicknameSetupV.dat

select 1 from dual where 1 = (select count(*) from custom_nicknames );
select 1 from dual where 1 = (select count(*) from custom_nicknames where nickname = 'Test_Phone Number' and network_id = 999);

select 1 from dual where 1 = (select count(*) from list_configs );
select 1 from dual where 1 = (select count(*) from list_configs where name = 'Test_Phone Number');

select 1 from dual where 1 = (select count(*) from scheduler_command_maps );
select 1 from dual where 1 = (select count(*) from scheduler_command_maps where job_name = 'Custom Nickname - Test' and command = 'customnicknameload_TestDAILYADDITION2.sh,TestPid2' and is_network_based=0);
spool off
EOF
echo "Here BNow"
if [ $? -ne  '0' ] || grep "no rows" testCustomNicknameSetupV.dat
then
	echo "At exit 3"
	exitval=3
else
	echo "At exit 0"
	exitval=0
fi 
rm -f testCustomNicknameSetupV.dat
exit $exitval

