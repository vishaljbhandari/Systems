#! /bin/bash
ls $WEBSERVER_RANGER_HOME/bin/customnicknameload_TestDAILYADDITION2.sh

if [ $? -ne 0 ] 
then
	exit 1
fi

cmp generated_customnicknamefile.txt $WEBSERVER_RANGER_HOME/bin/customnicknameload_TestDAILYADDITION2.sh

if [ $? -ne 0 ] 
then
	exit 2
fi
$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool testCustomNicknameSetupVStandalone.dat

select 1 from dual where 1 = (select count(*) from scheduler_command_maps );
select 1 from dual where 1 = (select count(*) from scheduler_command_maps where job_name = 'Custom Nickname - Test' and command = 'customnicknameload_TestDAILYADDITION2.sh,TestPid');

spool off
EOF

if [ $? -ne  '0' ] || grep "no rows" testCustomNicknameSetupVStandalone.dat
then
	exitval=3
else
	exitval=0
fi 
rm -f testCustomNicknameSetupVStandalone.dat
exit $exitval

