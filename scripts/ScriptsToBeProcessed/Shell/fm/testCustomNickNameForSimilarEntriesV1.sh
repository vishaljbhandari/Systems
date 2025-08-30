#! /bin/bash


ls $WEBSERVER_RANGER_HOME/bin/customnicknameload_NickDAILYADDITION2.sh

if [ $? -ne 0 ] 
then
	exit 1
fi

cmp generated_customnicknamefileforsimilar_nick.txt $WEBSERVER_RANGER_HOME/bin/customnicknameload_NickDAILYADDITION2.sh

if [ $? -ne 0 ] 
then
	exit 2
fi

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool testCustomNickNameForSimilarEntriesV1.dat

select 1 from dual where 2 = (select count(*) from custom_nicknames );
select 1 from dual where 1 = (select count(*) from custom_nicknames where nickname = 'Nick_Phone Number' and network_id = 999);

select 1 from dual where 2 = (select count(*) from list_configs );

select 1 from dual where 1 = (select count(*) from list_configs where name = 'Nick_Phone Number');

select 1 from dual where 2 = (select count(*) from scheduler_command_maps );
select 1 from dual where 1 = (select count(*) from scheduler_command_maps where job_name = 'Custom Nickname - Nick' and command = 'customnicknameload_NickDAILYADDITION2.sh,NickPid2' and is_network_based=0);
spool off
EOF

if [ $? -ne  '0' ] || grep "no rows" testCustomNickNameForSimilarEntriesV1.dat
then
	exitval=3
else
	exitval=0
fi 
rm -f testCustomNickNameForSimilarEntriesV1.dat 
exit $exitval

