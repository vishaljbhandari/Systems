#! /bin/bash

rm -f $COMMON_MOUNT_POINT/FMSData/NegativeRuleData/Process/NEG*
rm -f $COMMON_MOUNT_POINT/FMSData/NegativeRuleData/Process/success/NEG*

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from negative_rule_exec_status ;
delete from cdr ;
delete from negative_rule_queries where id = 1024 ;
delete from NETWORKS_RULES where rule_id in (7000) ; 

delete from rules where id in (7000, 7001) ;
delete from temp_negative_rule_data ;

insert into rules (id, parent_id, key, name, version, aggregation_type, is_enabled, severity, user_id, processor_type, is_active, category)
	values (7000, 1000, 7000, 'Negative Rule - 1', 0, 0, 1, 100, 'default', 0, 1, 'NEGATIVE_RULE') ;

insert into rules (id, parent_id, key, name, version, aggregation_type, is_enabled, severity, user_id, processor_type, is_active, category)
	values (7001, 1000, 7001, 'Negative Rule - 1', 0, 0, 1, 100, 'default', 0, 1, 'NEGATIVE_RULE') ;

insert into cdr (id,subscriber_id,caller_number,called_number,forwarded_number,record_type,duration,time_stamp, equipment_id,imsi_number,
		geographic_position,call_type,value,cdr_type, service_type, is_complete, is_attempted, network_id, phone_number, hour_of_day)
	values (1054,1810,'+919820336300','+919820535201','',1,10,to_date('29-12-2005', 'DD-MM-YYYY'), '490519820535201', '404209820535201',
		'404300006',1,10.5,1,1,1,0,1024,'+919820336300', 0) ;

insert into cdr (id,subscriber_id,caller_number,called_number,forwarded_number,record_type,duration,time_stamp, equipment_id,imsi_number,
		geographic_position,call_type,value,cdr_type, service_type, is_complete, is_attempted, network_id, phone_number, hour_of_day)
	values (1055,1910,'+919820336500','+919820535505','',1,10,to_date('29-12-2005', 'DD-MM-YYYY'), '490519820535500', '404209820535500',
		'404300006',1,10.5,1,1,1,0,1024,'+919820336500', 0);

insert into cdr (id,subscriber_id,caller_number,called_number,forwarded_number,record_type,duration,time_stamp, equipment_id,imsi_number,
		geographic_position,call_type,value,cdr_type, service_type, is_complete, is_attempted, network_id, phone_number, hour_of_day)
	values (1056,1910,'+919820336500','+919820535505','',1,10,to_date('29-12-2005', 'DD-MM-YYYY'), '490519820535500', '404209820535500',
		'404300006',1,10.5,1,1,1,0,1024,'+919820336500', 0);

insert into cdr (id,subscriber_id,caller_number,called_number,forwarded_number,record_type,duration,time_stamp, equipment_id,imsi_number,
		geographic_position,call_type,value,cdr_type, service_type, is_complete, is_attempted, network_id, phone_number, hour_of_day)
	values (1057,1910,'+919820336600','+919820535505','',1,10,to_date('29-12-2005', 'DD-MM-YYYY'), '490519820535500', '404209820535500',
		'404300006',1,10.5,1,1,1,0,1024,'+919820336600', 0);

insert into cdr (id,subscriber_id,caller_number,called_number,forwarded_number,record_type,duration,time_stamp, equipment_id,imsi_number,
		geographic_position,call_type,value,cdr_type, service_type, is_complete, is_attempted, network_id, phone_number, hour_of_day)
	values (1058,1910,'+919820336700','+919820535505','',1,10,to_date('29-12-2005', 'DD-MM-YYYY'), '490519820535500', '404209820535500',
		'404300006',1,10.5,1,1,1,0,1024,'+919820336700', 0) ;

INSERT INTO NETWORKS_RULES values(1024, 7000) ;

INSERT INTO negative_rule_queries values(1024, 7000, 'SELECT DISTINCT(phone_number), 2, network_id,  7000, id FROM CDR T', 
		'SELECT DISTINCT(phone_number), 2, network_id,  7000, id FROM CDR T') ;

INSERT INTO temp_negative_rule_data VALUES (1, 7000, 2, '+91', 1024) ;

commit;
 
exit;
EOF

