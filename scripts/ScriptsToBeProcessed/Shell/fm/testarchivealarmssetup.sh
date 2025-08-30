$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from alert_cdr ;
delete from alerts ;
delete from alarm_comments ;
delete from alarm_views_fraud_types ;
delete from matched_details ;
delete from matched_results ;
delete from mul_fld_func_res_in ;
delete from alarms ;
delete from cdr ;
delete from gprs_cdr ;
delete from reference_types_maps where reference_type = 9999 ;
delete from reference_types where id = 9999 ;

delete from archived_alert_cdr ;
delete from archived_alerts ;
delete from archived_alarm_comments ;
delete from archived_alarms_fraud_types ;
delete from archived_matched_details ;
delete from archived_matched_results ;
delete from ar_multi_field_func_res_info;
delete from archived_cdr ;
delete from archived_gprs_cdr ;
delete from archived_alarms ;

delete from fcn_generation_date ;
delete from rules_subscriber_groups ;
delete from networks_subscriber_groups ;
delete from subscriber_groups ;
delete from subscriber where id > 1024;
delete from account  where id >1024 ;

delete from analyst_actions_networks where analyst_action_id = 99999 ;
delete from analyst_actions where id = 99999 ;
delete from networks_rules where rule_id in (99999,99998) ;
delete from rules where id in (99999,99998) ;

insert into reference_types (ID, DESCRIPTION, RECORD_CONFIG_ID) values (9999, 'Test', null) ;
insert into analyst_actions (id, name) values(99999, 'Test Action');
insert into analyst_actions_networks (analyst_action_id, network_id) values (99999, 999) ;

insert into subscriber_groups (id, key, description, priority, record_config_id) values(1, 'PVN', 'Doctor', 1, 3);
insert into networks_subscriber_groups (network_id, subscriber_group_id)
	values (999, 1) ; 

insert into account (id, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code) values (1813,'18131960', to_date('', 'dd/mm/yyyy'),0, 'manu' ,'m' ,'kuman' ,'' ,'' ,'' ,'' ,'') ;

insert into subscriber( id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type) values (	1813, 1813, '+919820336303', to_date('', 'dd/mm/yyyy'),'' ,'' ,'' ,'' ,'' ,'404209820535200' , '' ,'1' ,'PVN' ,2047, 0,0, 1) ;

insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field, accumulation_function,is_enabled,created_time, modification_time, severity, user_id) values(99999,1,99999,'Test Rule',0,1,1,1,1,sysdate,sysdate,100,'radmin') ;
--Rule With No Alarms--
insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field, accumulation_function,is_enabled,created_time, modification_time, severity, user_id) values(99998,1,99998,'Un Used Test Rule',0,2,1,1,1,sysdate,sysdate,100,'radmin') ;

insert into alarms (network_id, id,rule_ids, reference_id, alert_count, created_date, modified_date, status, user_id, score, value, cdr_count, pending_time, reference_type, reference_value, is_visible) values (0, 99999, ' ', 1813, 1, sysdate - 20, sysdate - 0.2, 2, 'radmin', 10, 10, 1, NULL, 1, '+919820336303', 1); 
insert into alarm_comments (id, comment_date, user_id, alarm_id, action_id, user_comment) values (alarm_comments_seq.nextval, sysdate,'radmin',99999,99999,'no comments'); 
insert into alarm_views_fraud_types values (99999, 3) ;

insert into alerts (id, alarm_id, event_instance_id, value, cdr_count, repeat_count, cdr_time, created_date, modified_date, score, aggregation_type, aggregation_value, is_visible) values(99999, 99999, 99999, 10, 1, 6, sysdate - 20, sysdate - 20, sysdate, 10, 1, '+919820336303', 1);
insert into alert_cdr(alert_id,cdr_id,cdr_value,record_category) values(99999,1056,10,1);
insert into alert_cdr(alert_id,cdr_id,cdr_value,record_category) values(99999,1057,10,1);

insert into alert_cdr(alert_id,cdr_id,cdr_value,record_category) values(99999,1060,10,7);
insert into alert_cdr(alert_id,cdr_id,cdr_value,record_category) values(99999,1061,10,7);

insert into cdr (id,subscriber_id,phone_number,caller_number,called_number,forwarded_number,record_type,duration, time_stamp,equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete, is_attempted, network_id) values (1056,1813,'+919820336303','+919820535200','+919820535201','',1,10,sysdate - 3.0, '490519820535201', '404209820535201','404300006',1,10,1,1,1,0,0);

insert into cdr (id,subscriber_id,phone_number,caller_number,called_number,forwarded_number,record_type,duration, time_stamp,equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete, is_attempted, network_id) values (1057,1813,'+919820336303','+919820535200','+919820535202','',1,10,sysdate - 3.02, '490519820535202','404209820535202','404300006',1,10,1,1,1,0,0);

----Non Participating CDRs----
insert into cdr (id,subscriber_id,caller_number,called_number,forwarded_number,record_type,duration,time_stamp, equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete, is_attempted, network_id) values (1058,1813,'+919820535200','+919820535202','',1,10,sysdate - 3.04,'490519820535202', '404209820535202','404300006',1,10,1,1,1,0,0);

insert into gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type, upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id, imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year) values(1060,1024,1,sysdate-1,20,'+919820336303',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa', '333',to_char(sysdate-5, 'ddd'));
insert into gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type, upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id, imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year) values(1061,1024,1,sysdate-1,20,'+919820336303',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa', '333',to_char(sysdate-5, 'ddd'));
----Non Participating GPRS CDRs----
insert into gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type, upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id, imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year) values(1062,1024,1,sysdate-1,20,'+919820336308',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa', '333',to_char(sysdate-5, 'ddd'));

insert into matched_results (id, rule_key, aggregation_value, db_type, reference_id, reference_type) values (99998, 99998, '+919820336303',2, 1813, 2) ;
insert into matched_results (id, rule_key, aggregation_value, db_type, reference_id, reference_type) values (99999, 99999, '+919820336303',1, 1813, 2) ;

insert into matched_details (id, matched_results_id, matched_record, field_id, field_value, match_type) values (1, 99998, 0, 4, 'First Name', 1) ;
insert into matched_details (id, matched_results_id, matched_record, field_id, field_value, match_type) values (2, 99999, 1, 5, 'Middle Name', 1) ;


insert into mul_fld_func_res_in (id, rule_key, threshold_id, record_id, record_category, record_time, field_id, field_value, measured_value) values (1024, 99999,99999,1056,1,2,4,'First Name',1) ;
insert into mul_fld_func_res_in (id, rule_key, threshold_id, record_id, record_category, record_time, field_id, field_value, measured_value) values (1025, 99999,99999,1057,1,2,5,'Middle Name',1) ;

delete from configurations where config_key='ARCHIVER.ALARM.OPTION' ;
insert into configurations values (configurations_seq.nextval, 'ARCHIVER.ALARM.OPTION', 2, 0) ;

delete from configurations where config_key='ARCHIVER.LAST_ARCHIVED_TIME' ;
insert into configurations values (configurations_seq.nextval, 'ARCHIVER.LAST_ARCHIVED_TIME', to_char(sysdate - 1 , 'yyyy/mm/dd hh24:mi:ss'), 0) ;

delete from configurations where config_key = 'BULK_FETCH_ARRAY_SIZE' ;
insert into configurations values (configurations_seq.nextval, 'BULK_FETCH_ARRAY_SIZE', 1, 0) ;

delete from configurations where config_key = 'ARCHIVER.ALARM.BATCH_SIZE' ;
insert into configurations values (configurations_seq.nextval, 'ARCHIVER.ALARM.BATCH_SIZE', 2, 0) ;


commit;
EOF

if [ $? -ne 0 ];then
	exit 1
fi

echo "Test" > $COMMON_MOUNT_POINT/Attachments/Alarm/99999Data.txt

