rm $COMMON_MOUNT_POINT/Attachments/ArchivedAlarm/*
touch $COMMON_MOUNT_POINT/Attachments/ArchivedAlarm/99999ABC.txt

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

delete from alert_cdr ;
delete from alerts ;
delete from matched_details ;
delete from matched_results ;
delete from mul_fld_func_res_in ;
delete from case ;
delete from case_reason ;
delete from alarm_comments ;
delete from alarm_views_fraud_types ;
delete from alarms ;

delete from archived_alert_cdr ;
delete from archived_alerts ;
delete from archived_matched_details ;
delete from archived_matched_results ;
delete from ar_multi_field_func_res_info ;
delete from archived_alarm_comments ;
delete from archived_alarms_fraud_types ;
delete from archived_alarms ;

delete from cdr ;
delete from gprs_cdr ;

delete from archived_cdr ;
delete from archived_gprs_cdr ;
delete from archived_recharge_log ;

delete from analyst_actions_networks where analyst_action_id = 99999 ;
delete from analyst_actions where id = 99999 ;
delete from networks_rules where rule_id in (99999,99998) ;
delete from rules where id in (99999,99998,11111) ;

delete from configurations where config_key='BULK_FETCH_ARRAY_SIZE' ;
delete from configurations where config_key='CLEANUP.ARCHIVED.ALARMS.LAST_RUN_DATE' ;
delete from configurations where config_key='CLEANUP.ARCHIVED.ALARMS.INTERVAL_IN_DAYS' ;

insert into configurations values (configurations_seq.nextval, 'BULK_FETCH_ARRAY_SIZE', 2, 0) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.ARCHIVED.ALARMS.LAST_RUN_DATE', to_char((sysdate - 1),'YYYY/MM/DD HH24:MI:SS'), 0) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.ARCHIVED.ALARMS.INTERVAL_IN_DAYS', '0', 0) ;

insert into analyst_actions (id, name) values(99999, 'Test Action');
insert into analyst_actions_networks (analyst_action_id, network_id) values(99999, 999) ;
insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field,
	accumulation_function,is_enabled,created_time, modification_time, severity, user_id)
values(99999,1,99999,'Test Rule',0,1,1,1,1,sysdate,sysdate,100,'radmin') ;
insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field,
	accumulation_function,is_enabled,created_time, modification_time, severity, user_id)
values(99998,1,99998,'Un Used Test Rule',0,2,1,1,1,sysdate,sysdate,100,'radmin') ;
insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field,
	accumulation_function,is_enabled,created_time, modification_time, severity, user_id)
values(11111,1,11111,'Test Rule For Count Changes',0,2,1,11,1,sysdate,sysdate,100,'radmin') ;

insert into archived_alarms (network_id, id, reference_id, alert_count, created_date, modified_date, status, user_id,
					score, value, cdr_count, pending_time, reference_type, reference_value, is_visible, case_name, rule_ids)
values (0, 99999, 1813, 1, sysdate - 20, sysdate - 2, 2, 'radmin', 10, 10, 1, NULL, 1, '+919820336303', 1, 'test', 99999);
insert into archived_alarm_comments (id, comment_date, user_id, alarm_id, action_id, user_comment)
values (alarm_comments_seq.nextval, sysdate,'radmin',99999,99999,'no comments');
insert into archived_alarms_fraud_types values (99999, 3) ;

insert into archived_alerts (id, alarm_id, event_instance_id, value, cdr_count, repeat_count, cdr_time, created_date,
	modified_date, score, aggregation_type, aggregation_value, is_visible)
values(99999, 99999, 99999, 10, 1, 6, sysdate - 20, sysdate - 20, sysdate, 10, 1, '+919820336303', 1);

insert into archived_alerts (id, alarm_id, event_instance_id, value, cdr_count, repeat_count, cdr_time, created_date,
	modified_date, score, aggregation_type, aggregation_value, is_visible)
values(11111, 99999, 11111, 10, 1, 6, sysdate - 20, sysdate - 20, sysdate, 10, 1, '+919820336303', 1);

insert into archived_matched_results (id, rule_key, aggregation_value, db_type)
values (99999, 99999, '+919820336303',1) ;

insert into archived_matched_details (matched_results_id, matched_record, field_id, field_value, match_type)
values (99999, 0, 4, 'First Name', 1) ;
insert into archived_matched_details (matched_results_id, matched_record, field_id, field_value, match_type)
values (99999, 1, 5, 'Middle Name', 1) ;


insert into ar_multi_field_func_res_info (id, rule_key, threshold_id, record_id, record_category, record_time,
	field_id, field_value, measured_value)
values (1024,99999,99999,1056,1,2,4,'First Name',1) ;
insert into ar_multi_field_func_res_info (id, rule_key, threshold_id, record_id, record_category, record_time,
	field_id, field_value, measured_value)
values (1025,99999,99999,1057,1,2,5,'Middle Name',1) ;

insert into archived_cdr (id,subscriber_id,phone_number,caller_number,called_number,forwarded_number,record_type,duration,
	time_stamp,equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete,
	is_attempted, network_id,day_of_year,hour_of_day)
values (1056,1813,'+919820336303','+919820535200','+919820535201','',1,10,sysdate - 3.0,
	'490519820535201', '404209820535201','404300006',1,10,1,1,1,0,0,to_char(sysdate - 3.0,'DDD'),to_char(sysdate - 3.0,'HH'));
insert into archived_cdr (id,subscriber_id,phone_number,caller_number,called_number,forwarded_number,record_type,duration,
	time_stamp,equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete,
	is_attempted, network_id,day_of_year,hour_of_day)
values (1057,1813,'+919820336303','+919820535200','+919820535202','',1,10,sysdate - 3.02,
	'490519820535202','404209820535202','404300006',1,10,1,1,1,0,0,to_char(sysdate - 3.0,'DDD'),to_char(sysdate - 3.0,'HH'));

insert into archived_gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type,
	upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id,
	imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year)
values(1060,1024,1,sysdate-2,20,'+919820336303',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa',
	'333',to_char(sysdate-2, 'ddd'));
insert into archived_gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type,
	upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id,
	imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year)
values(1061,1024,1,sysdate-2,20,'+919820336303',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa',
	'333',to_char(sysdate-2, 'ddd'));

insert into archived_recharge_log (network_id, id, time_stamp, phone_number, amount, recharge_type, 
		status, credit_card, pin_number, serial_number,subscriber_id, account_id)
values (1024, 1080, sysdate-2, '+919820336303', 100, 1, 1, 'credit-card-1025', 'pin-no-1025', 
		'serial-no-1025',2837, 1816) ;

insert into archived_alert_cdr(alert_id,cdr_id,cdr_value,record_category) values (99999,1056,10,1);
insert into archived_alert_cdr(alert_id,cdr_id,cdr_value,record_category) values (99999,1057,10,1);
insert into archived_alert_cdr(alert_id,cdr_id,cdr_value,record_category) values (99999,1060,10,7);
insert into archived_alert_cdr(alert_id,cdr_id,cdr_value,record_category) values (99999,1061,10,7);
insert into archived_alert_cdr(alert_id,cdr_id,cdr_value,record_category) values (99999,1080,10,1);

insert into archived_alert_cdr(alert_id,cdr_id,cdr_value,record_category) values (11111,1056,10,3);
insert into archived_alert_cdr(alert_id,cdr_id,cdr_value,record_category) values (11111,1057,10,3);

--Non Participating Records---
insert into archived_alarms (network_id, id, reference_id, alert_count, created_date, modified_date, status, user_id,
					score, value, cdr_count, pending_time, reference_type, reference_value, is_visible, case_name, rule_ids)
values (1024, 99998, 1814, 1, sysdate - 20, sysdate - 1, 1, 'radmin', 10, 10, 1, NULL, 1, '+919820336302', 1, 'test', 99998);
insert into archived_alarm_comments (id, comment_date, user_id, alarm_id, action_id, user_comment)
values (alarm_comments_seq.nextval, sysdate,'radmin',99998,99998,'no comments');
insert into archived_alarms_fraud_types values (99998, 4) ;
insert into archived_alerts (id, alarm_id, event_instance_id, value, cdr_count, repeat_count, cdr_time, created_date,
	modified_date, score, aggregation_type, aggregation_value, is_visible)
values(99998, 99998, 99998, 10, 1, 6, sysdate - 20, sysdate - 20, sysdate, 10, 1, '+919820336303', 1);
insert into archived_matched_results (id, rule_key, aggregation_value, db_type)
values (99998, 99998, '+919820336303',2) ;
insert into archived_matched_details (matched_results_id, matched_record, field_id, field_value, match_type)
values (99998, 1, 5, 'First Name', 1) ;
insert into ar_multi_field_func_res_info (id, rule_key, threshold_id, record_id, record_category, record_time,
	field_id, field_value, measured_value)
values (1026,99998,99998,1058,1,2,4,'First Name',1) ;
insert into archived_alert_cdr(alert_id,cdr_id,cdr_value,record_category) values (99998,1058,10,7) ;
insert into archived_alert_cdr(alert_id,cdr_id,cdr_value,record_category) values (99998,1062,10,1) ;
insert into archived_alert_cdr(alert_id,cdr_id,cdr_value,record_category) values (99998,1081,10,1) ;
insert into archived_cdr (id,subscriber_id,caller_number,called_number,forwarded_number,record_type,duration,time_stamp,
	equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete,
	is_attempted, network_id,day_of_year,hour_of_day)
values (1058,1813,'+919820535200','+919820535203','',1,10,sysdate + 90,'490519820535202',
	'404209820535202','404300006',1,10,1,1,1,0,0,to_char(sysdate + 90,'DDD'),to_char(sysdate + 90,'HH')) ;
insert into archived_gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type,
	upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id,
	imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year)
values(1062,1024,1,sysdate,20,'+919820336308',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa',
	'333',to_char(sysdate, 'ddd')) ;
insert into archived_recharge_log (network_id, id, time_stamp, phone_number, amount, recharge_type, 
		status, credit_card, pin_number, serial_number,subscriber_id, account_id)
values (1024, 1081, sysdate-2, '+919820336301', 100, 1, 1, 'credit-card-1025', 'pin-no-1025', 
		'serial-no-1025',2837, 1816) ;

commit ;
quit ;
EOF
