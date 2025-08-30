#! /bin/sh

AT_SUBSCRIBER=1
HOSTNAME=`hostname`

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool calledbyalarmgenerationV.dat

select 1 from dual where 1 = (select count(*) from alarms
				where reference_type = 1
					and reference_value = '+919845200001'                             
					and reference_id = 1025
					and alert_count = 1
					and cdr_count = 6
					and rule_tags = '1026,1027,1028' 
					and fraud_types = '1026,1027,1028' 
					and network_id = 1024) ;


select 1 from dual where 1 = (select count(*) from alerts where aggregation_value = '+919845200001');

select 1 from dual where 1 = (select count(*) from alarms
				where reference_type = 1
					and reference_value = '+919845200004'                             
					and reference_id = 1028
					and alert_count = 1
					and cdr_count = 6
					and rule_tags = '1026,1027,1028' 
					and fraud_types = '1026,1027,1028' 
					and network_id = 1024) ;

select 1 from dual where 1 = (select count(*) from alarms
				where reference_type = 1
					and reference_value = '+919845200002'                             
					and reference_id = 1026
					and alert_count = 2
					and cdr_count = 7
					and rule_tags = '1026,1027,1028' 
					and fraud_types = '1026,1027,1028' 
					and network_id = 1024) ;


select 1 from dual where 1 = (select count(*) from alarms
				where alert_count = 2
					and cdr_count = 7 
					and network_id = 1024
					and reference_id = 1026) ;

select 1 from dual where 1  = (select count(*) from audit_trails
		                     where ENTITY_VALUE = '+919845200004'
							 and IP_ADDRESS = '$HOSTNAME'
							 and source = 1
							 and network_id = '1024');

		                      
select 1 from dual where 2 = (select count(*) from alerts where aggregation_value = '+919845200002');
select 1 from dual where 1 = (select count(*) from audit_trails where  entity_type=3001 and entity_value='+919845200001' and
	network_id = '1024');
select 1 from dual where 1 = (select count(*) from audit_trails where  entity_type=3001 and entity_value='+919845200004' and
	network_id = '1024');
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
