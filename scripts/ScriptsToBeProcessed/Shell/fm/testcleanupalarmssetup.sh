rm $COMMON_MOUNT_POINT/Attachments/Alarm/*
rm $COMMON_MOUNT_POINT/Attachments/Case/*

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from configurations where config_key='BULK_FETCH_ARRAY_SIZE' ;
delete from configurations where config_key='CLEANUP.ALARMS.OPTION' ;
delete from configurations where config_key='CLEANUP.ALARMS.LAST_RUN_DATE' ;
delete from configurations where config_key='CLEANUP.ALARMS.INTERVAL_IN_DAYS' ;
delete from configurations where config_key='CLEANUP.ALARMS.BATCH_VALUE' ;
delete from configurations where config_key='CLEANUP.RECORDS.OPTION' ;
delete from configurations where config_key='CLEANUP.RECORDS.CDR.OPTIONS' ;
delete from configurations where config_key='CLEANUP.RECORDS.GPRS_CDR.OPTIONS' ;

insert into configurations values (configurations_seq.nextval, 'BULK_FETCH_ARRAY_SIZE', 2, 1) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.ALARMS.OPTION', '2,4', 1) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.ALARMS.LAST_RUN_DATE', to_char(trunc(sysdate - 1),
'YYYY/MM/DD HH24:MI:SS'), 1) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.ALARMS.INTERVAL_IN_DAYS', '0', 1) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.ALARMS.BATCH_VALUE', '2', 1) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.OPTION', '1,7', 1) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.CDR.OPTIONS', to_char(trunc(sysdate - 1),'YYYY/MM/DD HH24:MI:SS')||',1', 1) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.GPRS_CDR.OPTIONS', to_char(trunc(sysdate - 1),'YYYY/MM/DD HH24:MI:SS')||',1', 1) ;

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
delete from alarm_cleanup ;

delete from archived_alert_cdr ;
delete from archived_alerts ;
delete from archived_matched_details ;
delete from archived_matched_results ;
delete from ar_multi_field_func_res_info;
delete from archived_alarm_comments ;
delete from archived_alarms_fraud_types ;
delete from archived_alarms ;

delete from cdr ;
delete from archived_cdr ;
delete from gprs_cdr ;
delete from archived_gprs_cdr ;

delete from analyst_actions_networks where analyst_action_id = 99999 ;
delete from analyst_actions where id = 99999 ;
delete from networks_rules where rule_id in (99999,99998) ;
delete from rules where id in (99999,99998) ;
delete from fp_profiles_16_1 where profiled_entity_id = 1810 and entity_id = 16;
delete from fp_profiles_16_2 where profiled_entity_id = 1810 and entity_id = 16;
delete from fp_suspect_profiles_16 where profiled_entity_id = 1810 and entity_id = 16;

insert into analyst_actions (id, name) values(99999, 'Test Action');
insert into analyst_actions_networks (analyst_action_id, network_id) values(99999, 999) ;
insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field,
	accumulation_function,is_enabled,created_time, modification_time, severity, user_id)
values(99999,1,99999,'Test Rule',0,1,1,1,1,sysdate,sysdate,100,'radmin') ;
insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field,
	accumulation_function,is_enabled,created_time, modification_time, severity, user_id)
values(99998,1,99998,'Un Used Test Rule',0,2,1,1,1,sysdate,sysdate,100,'radmin') ;

insert into alarms (network_id, id,rule_ids, reference_id, alert_count, created_date, modified_date, status, user_id, owner_type, score, value, cdr_count, pending_time, reference_type, reference_value, is_visible, case_id) values (0, 99999, ' ',1813, 1, sysdate - 20, sysdate - 2, 2, 'radmin', 0, 10, 10, 1, NULL, 1, '+919820336303', 1, 1025);
insert into alarm_comments (id, comment_date, user_id, alarm_id, action_id, user_comment)
values (alarm_comments_seq.nextval, sysdate,'radmin',99999,99999,'no comments');
insert into alarm_views_fraud_types values (99999, 3) ;

insert into case_reason (id, reason, description ) values (1, 'GlobalReason1','GlobalReason1');
insert into case (id, network_id, name, description, owner_id, created_date, modified_date, status,	reason_id, creator_id, alarm_count)
		values (1025, 1024, 'case0', 'Case 0', 'radmin', sysdate - 1, sysdate, 0, 1, 'radmin', 1);
insert into case (id, network_id, name, description, owner_id, created_date, modified_date, status,	reason_id, creator_id, alarm_count)
		values (1026, 1024, 'case1', 'Case 1', 'radmin', sysdate - 3, sysdate - 2, 2, 1, 'radmin', 1);

insert into alerts (id, alarm_id, event_instance_id, value, cdr_count, repeat_count, cdr_time, created_date,
	modified_date, score, aggregation_type, aggregation_value, is_visible)
values(99999, 99999, 99999, 10, 1, 6, sysdate - 20, sysdate - 20, sysdate, 10, 1, '+919820336303', 1);
insert into alert_cdr(alert_id,cdr_id,cdr_value,record_category) values(99999,1056,10,1);
insert into alert_cdr(alert_id,cdr_id,cdr_value,record_category) values(99999,1057,10,1);

insert into alert_cdr(alert_id,cdr_id,cdr_value,record_category) values(99999,1060,10,7);
insert into alert_cdr(alert_id,cdr_id,cdr_value,record_category) values(99999,1061,10,7);

insert into cdr (id,subscriber_id,phone_number,caller_number,called_number,forwarded_number,record_type,duration,
	time_stamp,equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete,
	is_attempted, network_id,day_of_year,hour_of_day)
values (1056,1813,'+919820336303','+919820535200','+919820535201','',1,10,sysdate - 3.0,
	'490519820535201', '404209820535201','404300006',1,10,1,1,1,0,0,to_char(sysdate - 3.0,'DDD'),to_char(sysdate - 3.0,'HH'));
insert into cdr (id,subscriber_id,phone_number,caller_number,called_number,forwarded_number,record_type,duration,
	time_stamp,equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete,
	is_attempted, network_id,day_of_year,hour_of_day)
values (1057,1813,'+919820336303','+919820535200','+919820535202','',1,10,sysdate - 3.02,
	'490519820535202','404209820535202','404300006',1,10,1,1,1,0,0,to_char(sysdate - 3.0,'DDD'),to_char(sysdate - 3.0,'HH'));
--Non Participating CDRs----
insert into cdr (id,subscriber_id,caller_number,called_number,forwarded_number,record_type,duration,time_stamp,
	equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete,
	is_attempted, network_id,day_of_year,hour_of_day)
values (1058,1813,'+919820535200','+919820535202','',1,10,sysdate + 90,'490519820535202',
	'404209820535202','404300006',1,10,1,1,1,0,0,to_char(sysdate + 90,'DDD'),to_char(sysdate + 90,'HH')) ;

insert into matched_results (id, rule_key, aggregation_value, db_type, reference_id, reference_type)
values (99998, 99998, '+919820336303',2, 1813, 1) ;
insert into matched_results (id, rule_key, aggregation_value, db_type, reference_id, reference_type)
values (99999, 99999, '+919820336303',1, 1813, 1) ;

insert into matched_details (id, matched_results_id, matched_record, field_id, field_value, match_type)
values (1, 99999, 0, 4, 'First Name', 1) ;
insert into matched_details (id, matched_results_id, matched_record, field_id, field_value, match_type)
values (2, 99999, 1, 5, 'Middle Name', 1) ;


insert into mul_fld_func_res_in (id, rule_key, threshold_id, record_id, record_category, record_time,
	field_id, field_value, measured_value)
values (1024, 99999,99999,1056,1,2,4,'First Name',1) ;
insert into mul_fld_func_res_in (id, rule_key, threshold_id, record_id, record_category, record_time,
	field_id, field_value, measured_value)
values (1025, 99999,99999,1057,1,2,5,'Middle Name',1) ;

insert into gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type,
	upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id,
	imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year)
values(1060,1024,1,sysdate-2,20,'+919820336303',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa',
	'333',to_char(sysdate-2, 'ddd'));
insert into gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type,
	upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id,
	imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year)
values(1061,1024,1,sysdate-2,20,'+919820336303',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa',
	'333',to_char(sysdate-2, 'ddd'));
--Non Participating GPRS CDRs----
insert into gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type,
	upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id,
	imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year)
values(1062,1024,1,sysdate,20,'+919820336308',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa',
	'333',to_char(sysdate, 'ddd'));

delete from FP_PROFILES_16_1 ;
delete from FP_PROFILES_16_2 ;
delete from FP_PROFILES_221_1 ;
delete from FP_PROFILES_221_2 ;
delete from FP_SUSPECT_PROFILES_16 ;
delete from FP_SUSPECT_PROFILES_221 ;

INSERT INTO FP_SUSPECT_PROFILES_16 (ID, ENTITY_ID, PROFILED_ENTITY_ID, GENERATED_DATE, SERIAL_NUMBER, ELEMENTS, IS_MATCH_TYPE, VERSION) VALUES (5000, 16, 1810, sysdate, 1, '5000;5001;5002;5003;5004|+919886712342(30);+919884414376(21);+016705195378(4);+912349856784(13)|+916239473334(9);+119356294747(26);+910987778645(41);+117678934566(83)|http://www.mail.yahoo.com/login.html(70);http://wwww.subexa', 1, 0) ;
INSERT INTO FP_SUSPECT_PROFILES_16 (ID, ENTITY_ID, PROFILED_ENTITY_ID, GENERATED_DATE, SERIAL_NUMBER, ELEMENTS, IS_MATCH_TYPE, VERSION) VALUES (5001, 16, 1810, sysdate, 2, 'zure.com/products/nikirav6.2.html(13);http://www.wikipedia.com/en/quantam_mechanics.html(15);http://www.google.co.in(288)|122|5', 1, 0) ;
INSERT INTO FP_SUSPECT_PROFILES_16 (ID, ENTITY_ID, PROFILED_ENTITY_ID, GENERATED_DATE, SERIAL_NUMBER, ELEMENTS, IS_MATCH_TYPE, VERSION) VALUES (5002, 16, 1810, sysdate, 1, '5000;5001;5002;|+919886712342(40);+919884414376(61);+016705195378(8);+912349856784(83)|+916239473334(9);+119356294747(216);+910987778645(241);+117678934566(37)|http://www.mail.cyrex.com/login.html(30);http://www.x.com(9)', 1, 1) ;

-- Workflow Cleanup --
    delete from alarm_activity_logs ;
    delete from workflow_transactions ;
    delete from activities ;
	delete from activity_templates_actions ;
    delete from activity_templates where id = 1301 ;

    insert into activity_templates (id, name, description, template_type)
        values (1301, 'Standard Template 12', 'Standard Template 1', 0) ;

    insert into activities (id, name, description, jeopardy_period, activity_template_id, is_automatic)
      values (1024, 'Activity 1024', 'Activity 1024', 10, 1301, 0) ;

    insert into activities (id, name, description, jeopardy_period, activity_template_id, is_automatic)
      values (1025, 'Activity 1025', 'Activity 1025', 10, 1301, 0) ;

    insert into activities (id, name, description, jeopardy_period, activity_template_id, is_automatic)
      values (1026, 'Activity 1026', 'Activity 1026', 10, 1301, 0) ;

    insert into workflow_transactions (id, alarm_id, activity_id, user_id, expected_completion_date,
                                                    status, position, is_mandatory)
                values (1024, 99999, 1024, 1, sysdate, 0, 1, 1) ;

    insert into workflow_transactions (id, alarm_id, activity_id, user_id, expected_completion_date,
                                                    status, position, is_mandatory)
                values (1025, 99999, 1025, 1, sysdate, 0, 1, 1) ;

    insert into alarm_activity_logs values (2222, 99999, 1, 'Some Activity', sysdate + 0, 'Some Activitie''s Log Message 1', 'nadmin');

-- End Workflow Cleanup --

commit;
EOF

if [ $? -ne 0 ];then
	exit 1
fi

echo "Test" > $COMMON_MOUNT_POINT/Attachments/Alarm/99999Data.txt
echo "Test" > $COMMON_MOUNT_POINT/Attachments/Case/99999Data.txt
