#! /bin/sh

AT_SUBSCRIBER=1
INTERNAL_ACTION=0

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool calltocalledbyalarmgenerationV.dat

select 1 from dual where 1 = (select count(*) from alarms
								where  reference_type = 1 
									and reference_value = '+919845200001'                             
									and alert_count = 1
									and cdr_count = 6
									and network_id = 1024) ;

select 1 from dual where 1 = (select count(*) from alerts where aggregation_type = 2 and aggregation_value = '+919845200001');

select 1 from dual where 1 = (select count(*) from alarms
									where  reference_type = 1 
										and reference_value = '+919845200004'                             
										and alert_count = 1
										and cdr_count = 6
										and network_id = 1024) ;

select 1 from dual where 0 < (select count(*) from alarm_comments
									where action_id = $INTERNAL_ACTION
									and user_comment like 'Source: Blacklisted Number +919845200003');
									
--select 1 from dual where 1 = (select count(*) from fms_log
									--where action like 'The Alarm for Phone Number +919845200004 is New'
									--and fms_log_id = 1);

select 1 from dual where 1 = (select count(*) from alerts where  aggregation_type = 2 and aggregation_value = '+919845200004');

select 1 from dual where 1 = (select count(*) from alarms
									where  reference_type = 1 
										and reference_value = '+919845200002'                             
										and alert_count = 2
										and cdr_count = 7 
										and network_id = 1024
										and trunc(modified_date) = trunc(sysdate)
										and is_visible = 1
										and status = 0);

select 1 from dual where 2 = (select count(*) from alerts
									where  aggregation_type = 2 and aggregation_value = '+919845200002');

select 1 from dual where 1 = (select count(*) from alerts
									where  aggregation_type = 2 
										and aggregation_value = '+919845200002'
										and repeat_count = 1
										and cdr_count = 7
										and trunc(modified_date) = trunc(sysdate)
										and is_visible = 1);

select 1 from dual where 0 = NVL((select count(*) from alarms where  reference_type = 1 and reference_value='+919845200005'), 0);
select 1 from dual where 0 = NVL((select count(*) from alarms where  reference_type = 1 and reference_value='+919845002390'), 0);

select 1 from dual where 19 = (select count(*) from alert_cdr);

select 1 from dual where 1 = (select count(*) from audit_trails where  entity_type=3001 and entity_value='+919845200001' and
	network_id = '1024');
select 1 from dual where 1 = (select count(*) from audit_trails where  entity_type=3001 and entity_value='+919845200004' and
	network_id = '1024');
spool off
EOF

if [ $? -eq '5' ] || grep "no rows" calltocalledbyalarmgenerationV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f calltocalledbyalarmgenerationV.dat
exit $exitval
