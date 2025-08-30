#! /bin/sh

AT_SUBSCRIBER=1

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool calledbyalarmgenerationV.dat

select 1 from dual where 1 = (select count(*) from alarms 
								where reference_id = 1026
								and alert_count = 3
								and score = 95);

select 1 from dual where 2 = (select count(*) from alerts where aggregation_type = 2 and aggregation_value='+91173750022');
select 1 from dual where 3 = (select count(*) from alerts where alarm_id in (select id from alarms where reference_id = 1026));

select 1 from dual where 1 = (select count(*) from alerts 
		where aggregation_type = 2 and aggregation_value='+91173750022' and event_instance_id=2025);

select 1 from dual where 1 = (select count(*) from alerts 
		where aggregation_type = 2 and aggregation_value='+91173750022' and event_instance_id=2024);

select 1 from dual where 6 = (select count(*) from alert_cdr where alert_id = (select id from alerts 
		where aggregation_type=2 and aggregation_value='+91173750022' and event_instance_id=2024)) ;

select 1 from dual where 6 = (select count(*) from alert_cdr where alert_id = (select id from alerts where aggregation_type=2 and aggregation_value='+91173750022' and event_instance_id=2025)) ;

select 1 from dual where 0 = (select count(*) from alert_cdr 
		where cdr_id in (select cdr_id from alert_cdr 
			where alert_id in (select id from alerts 
				where aggregation_type=2 and aggregation_value='+91173750022' and event_instance_id=2024)) 
					and alert_id=(select id from alerts 
					where aggregation_type=2 and aggregation_value='+91173750022' and event_instance_id=2025));

select 1 from dual where 1 = (select distinct record_type from cdr 
		where id in (select cdr_id from alert_cdr 
			where alert_id in (select id from alerts
				where aggregation_type=2 and aggregation_value='+91173750022' and event_instance_id=2024)));

select 1 from dual where 2 = (select distinct record_type from cdr 
		where id in (select cdr_id from alert_cdr 
			where alert_id in (select id from alerts
				where aggregation_type=2 and aggregation_value='+91173750022' and event_instance_id=2025)));
spool off
EOF

if [ $? -eq '5' ] || grep "no rows" calledbyalarmgenerationV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f calledbyalarmgenerationV.dat
exit $exitval
