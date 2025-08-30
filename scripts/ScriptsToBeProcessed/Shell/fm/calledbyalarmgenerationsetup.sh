#! /bin/bash

CALLED_BY_EVENT_ID=2024
SUBSCRIBER_TYPE_FRAUDULENT=1

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from cdr;
delete from subscriber_profile_dates ;

delete from networks_subscriber_groups ;
delete from subscriber_groups where key = 'GOLDEN';
delete subscriber where id > 1024 ;
delete from fcn_generation_date ;
delete account where id > 1024 ;

delete from networks_rules where rule_id in (select id from rules where pseudo_function_id in (22,23)) ;
delete from record_configs_rules where rule_id in (select id from rules where pseudo_function_id in (22,23)) ;
delete from actions_rules where rule_id in (select id from rules where pseudo_function_id in (22,23)) ;
delete from analyst_actions_rules where rule_id in (select id from rules where pseudo_function_id in (22,23)) ;
delete from rules_tags where rule_id in (select id from rules where pseudo_function_id in (22,23)) ;
delete from rule_priority_maps where rule_id in (select id from rules where pseudo_function_id in (22,23)) ;
delete from networks_rules where rule_id in (select id from rules where pseudo_function_id in (22,23)) ;


delete from fraud_types_rules where  fraud_type_id in (1026,1027,1028) ;
delete from rules_tags where tag_id in (1026,1027,1028) ;
delete from tags where  id in (1026,1027,1028) ;


delete from rules where id in (select id from rules where pseudo_function_id in (22,23)) ;
delete from thresholds where id in (2024, 2025) ;
delete from components_pseudo_functions  where pseudo_function_id in (22,23); 
delete from pseudo_functions where id in (22, 23) ;

delete from alarm_comments;
delete from alerts;
delete from alert_cdr;
delete from alarms;

delete from fms_log where FMS_LOG_ID = 1;
delete from temp_call_to_by_alert_info;
delete from audit_trails ;

delete from blacklist_profile_options ;
drop sequence blacklist_profile_options_seq ;
create sequence blacklist_profile_options_seq increment by 1 nomaxvalue minvalue 1 nocycle cache 20 order ;

delete from configurations where config_key = 'LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024';
delete from configurations where config_key = 'LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024';
delete from configurations where config_key = 'STOP_LSP_BULK_PROCESSING_ST_0_PT_0_1024' ;
delete from configurations where config_key = 'STOP_LSP_BULK_PROCESSING_ST_1_PT_1_1024' ;

insert into configurations values (configurations_seq.nextval, 'LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024', '1024', 0);
insert into configurations values (configurations_seq.nextval, 'LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024', '0', 0);
insert into configurations values (configurations_seq.nextval, 'STOP_LSP_BULK_PROCESSING_ST_0_PT_0_1024', 'TRUE', 0);
insert into configurations values (configurations_seq.nextval, 'STOP_LSP_BULK_PROCESSING_ST_1_PT_1_1024', 'TRUE', 0);

insert into pseudo_functions (id, name, accumulation_function_id, accumulation_field_id) Values (22, 'Test Called To', 0, 2) ;
insert into pseudo_functions (id, name, accumulation_function_id, accumulation_field_id) Values (23, 'Test Called By', 0, 2) ;

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number, day_of_year )
values ( cdr_seq.nextval, 1024, '+919845200002', '+919845200001', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1026, 10, 1, 1, 1, 1, 0, 2, '+919845200002' , 224); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200001', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number )
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200001', null, 1, 40,  to_date( '09/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200001', null, 1, 40,  to_date( '11/10/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200001', null, 1, 40,  to_date( '11/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200001', null, 1, 40,  to_date( '11/12/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200001', null, 1, 40,  to_date( '11/13/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number, day_of_year ) 
values ( cdr_seq.nextval, 1024, '+919845200002', '+919845200001', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1026, 10, 1, 1, 1, 1, 0, 2, '+919845200002' , 224); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number, day_of_year )
values ( cdr_seq.nextval, 1024, '+919845200001', '+919845200002', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1025, 10, 1, 1, 1, 1, 0, 2, '+919845200001' , 224); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200004', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200004', null, 1, 40,  to_date( '09/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200004', null, 1, 40,  to_date( '11/10/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200004', null, 1, 40,  to_date( '11/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
	'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200004', null, 1, 40,  to_date( '11/12/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number)
values ( cdr_seq.nextval, 1024, '+919845200003', '+919845200004', null, 1, 40,  to_date( '11/13/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am'),
		'404212000002', '504220000002', '404300004', 1, 1027, 10, 8, 1, 1, 1, 0, 2, '+919845200003'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number) 
	select cdr_seq.nextval, network_id, caller_number, '+919845200002', forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, 1027, value, cdr_type, service_type, cdr_category, is_complete, is_attempted, 
		service, phone_number
	from cdr where called_number = '+919845200004' ;


insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, is_attempted, 
		service, phone_number) 
	select cdr_seq.nextval, network_id, caller_number, '+919845200005', forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, 10291, value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service,
		phone_number from cdr where called_number = '+919845200004' ;

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
		duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
		value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number)
	select cdr_seq.nextval, network_id, caller_number, '+919845002390', forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, 10292, value, cdr_type, service_type, cdr_category, is_complete, is_attempted, 
		service, phone_number
	from cdr where called_number = '+919845200004' ;


update cdr set day_of_year =  to_char(sysdate - 3, 'ddd') ;
update cdr set time_stamp =  sysdate ;


insert into account (id, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, 
		address_line_3, city, post_code)
values (1025,'a.0000001040', to_date('01/01/2003', 'dd/mm/yyyy'),1024, 'ahmed' ,'abdul kader ali' ,'abdo' ,'aljzair' ,'libia cente' ,
		'sana''a' ,'sana''a, yemen' ,'1848') ;

insert into account (id, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, 
		address_line_3, city, post_code)
values (1026,'a.0000001041', to_date('01/01/2003', 'dd/mm/yyyy'),1024, 'ram' ,'shyam' ,'ram' ,'bala' ,'' ,'' ,'sana''a, yemen' ,'1848') ;

insert into account (id, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, 
		address_line_3, city, post_code)
values (1027,'a.0000001041', to_date('01/01/2003', 'dd/mm/yyyy'),1024, 'ram' ,'shyam' ,'ram' ,'bala' ,'' ,'' ,'sana''a, yemen' ,'1848') ;

insert into account (id, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, 
		address_line_3, city, post_code)
values (1028,'a.0000001041', to_date('01/01/2003', 'dd/mm/yyyy'),1024, 'ed' ,'dul kad a' ,'abdo' ,'aljzair' ,'la cente' ,'sana''a' ,
		'sana''a, yemen' ,'1848') ;

insert into account (id, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, 
		address_line_3, city, post_code)
values (1029,'a.0000001041', to_date('01/01/2003', 'dd/mm/yyyy'),1025, 'ed' ,'dul kad a' ,'abdo' ,'aljzair' ,'la cente' ,'sana''a' ,
		'sana''a, yemen' ,'1848') ;

insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, 
		mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) 
values (1025, 1025, '+919845200001', to_date('01/01/2003', 'dd/mm/yyyy'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025' ,
		'504220000001' ,'404212000002' ,'1' ,'golden' ,4095, 1,1, 1, 1024) ;

insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, 
		mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id)
values (1026, 1026, '+919845200002', to_date('01/01/2003', 'dd/mm/yyyy'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025' ,
		'504220000001' ,'404212000002' ,'1' ,'golden' ,4095, 1,1, 1, 1024) ;

insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, 
		mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id, subscriber_type)
values (1027, 1027, '+919845200003', to_date('01/01/2003', 'dd/mm/yyyy'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025' ,
		'504220000001' ,'404212000002' ,'0' ,'golden' ,4095, 0,1, 1, 1024, $SUBSCRIBER_TYPE_FRAUDULENT) ;

insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, 
		mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id)
values (1028, 1028, '+919845200004', to_date('01/01/2003', 'dd/mm/yyyy'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025',
		'504220000001' ,'404212000002' ,'1' ,'golden' ,4095, 1,1, 1, 1024) ;

insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, 
		mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id, subscriber_type)
values (10291, 1029, '+919845200005', to_date('01/01/2003', 'dd/mm/yyyy'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025',
		'504220000001' ,'404212000002' ,'0' ,'golden' ,4095, 0,1, 1, 1024, $SUBSCRIBER_TYPE_FRAUDULENT) ;

insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, 
		mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id, subscriber_type)
values (10292, 1029, '+919845002390', to_date('01/01/2003', 'dd/mm/yyyy'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025' ,
		'504220000001' ,'404212000002' ,'3' ,'golden' ,4095, 0,1, 1, 1024, $SUBSCRIBER_TYPE_FRAUDULENT) ;

insert into blacklist_profile_options select BLACKLIST_PROFILE_OPTIONS_SEQ.nextval, id, PHONE_NUMBER, 50, network_id from subscriber where status = 0;
insert into blacklist_profile_options select BLACKLIST_PROFILE_OPTIONS_SEQ.nextval, id, PHONE_NUMBER, 51, network_id from subscriber where status = 0;

insert into subscriber_groups ( id, key,  description, priority, record_config_id ) 
	values (1024, 'GOLDEN', 'GOLDEN', 1, 3); 

insert into networks_subscriber_groups (network_id, subscriber_group_id)
	values (1024, 1024) ; 
 
insert into alarms(network_id, id, reference_id, alert_count, created_date, modified_date, status, user_id, score, value, 
		cdr_count, pending_time, is_visible, rule_ids, reference_type, reference_value)
values (1024, alarms_seq.nextval, 1026, 2, sysdate - 20, sysdate - 2, 1, 'radmin', 10, 10, 1, NULL, 0, 
		'1030', 1, '+919845200002');

insert into alerts(id, alarm_id, event_instance_id, value, cdr_count, repeat_count, cdr_time, created_date, modified_date, score, 
		aggregation_type, aggregation_value, is_visible)
values (1023, alarms_seq.currval, 1028, 10, 1, 6, sysdate - 20, sysdate - 20, sysdate, 10, 2, '+919845200002', 1);

insert into alerts(id, alarm_id, event_instance_id, value, cdr_count, repeat_count, cdr_time, created_date, modified_date, score,
			aggregation_type, aggregation_value, is_visible)
values (1024, alarms_seq.currval, $CALLED_BY_EVENT_ID, 10, 1, 0, sysdate - 20, sysdate - 20, sysdate, 10, 2, '+919845200002', 0);

insert into alert_cdr (alert_id, cdr_id, cdr_value, record_category) values (1024, 0, 10, 1) ;

insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field, accumulation_function, pseudo_function_id, 
		is_enabled, created_time, modification_time, severity, user_id, description, processor_type, category,is_active)
values (1024, 0, 1024, 'Called By Template', 0, 2, 12, 7, 23, 1, sysdate, sysdate, 100, 'radmin', '', 1, 'Testing', 1) ;

insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field, accumulation_function, pseudo_function_id, 
		is_enabled, created_time, modification_time, severity, user_id, description, processor_type, category,is_active)
values (2024, 1024, 2024, 'Called By Rule', 1, 2, 17, 1, 23, 1, sysdate, sysdate, 100, 'radmin', 'Testing Rule', 1, 'Testing', 1) ;


insert into fraud_types_rules(rule_id, fraud_type_id) values (1024, 1026) ;
insert into tags(id, name, category) values (1026, 'Dummy Tag1', 'Test') ;
insert into rules_tags(rule_id, tag_id) values (2024, 1026) ;

insert into fraud_types_rules(rule_id, fraud_type_id) values (1024, 1027) ;
insert into tags(id, name, category) values (1027, 'Dummy Tag2', 'Test') ;
insert into rules_tags(rule_id, tag_id) values (2024, 1027) ;

insert into fraud_types_rules(rule_id, fraud_type_id) values (1024, 1028) ;
insert into tags(id, name, category) values (1028, 'Dummy Tag3', 'Test') ;
insert into rules_tags(rule_id, tag_id) values (2024, 1028) ;



insert into thresholds (id, threshold_id, aggregation_type, aggregation_value, rule_key, rule_version, value, version, start_date, 
		end_date,  start_time, end_time, duration, effective_time, effective_day, effective_week_day, is_active, changed_status) 
values (2024, 0, 2, '', 2024, 1, 5, 0, '', '', '', '', 0, 1, 1, 0, 1, 0) ;

insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field, accumulation_function, pseudo_function_id, 
		is_enabled, created_time, modification_time, severity, user_id, description, processor_type, category,is_active)
values (2023, 1024, 2024, 'Duplicate/Old Called By Rule', 0, 2, 17, 1, 23, 1, sysdate, sysdate, 100, 'radmin', 'Testing Rule', 1, 'Testing', 1) ;

insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field, accumulation_function, pseudo_function_id, 
		is_enabled, created_time, modification_time, severity, user_id, description, processor_type, category,is_active)
values (1025, 0, 1025, 'Called To Template', 0, 2, 12, 7, 22, 0, sysdate, sysdate, 100, 'radmin', '', 1, 'Testing', 1) ;

insert into rules (id, parent_id, key, name, version, aggregation_type, accumulation_field, accumulation_function, pseudo_function_id, 
		is_enabled, created_time, modification_time, severity, user_id, description, processor_type, category,is_active)
values (2025, 1025, 2025, 'Called To Rule', 0, 2, 17, 1, 22, 0, sysdate, sysdate, 100, 'radmin', 'Testing Rule', 1, 'Testing', 1) ;

insert into thresholds (id, threshold_id, aggregation_type, aggregation_value, rule_key, rule_version, value, version, start_date, 
		end_date,  start_time, end_time, duration, effective_time, effective_day, effective_week_day, is_active, changed_status) 
values (2025, 0, 2, '', 2025, 0, 5, 0, '', '', '', '', 0, 1, 1, 0, 1, 0) ;

insert into networks_rules (network_id, rule_id) values (1024, 1024) ;
insert into networks_rules (network_id, rule_id) values (1024, 1025) ;
insert into networks_rules (network_id, rule_id) values (1024, 2024) ;
insert into networks_rules (network_id, rule_id) values (1024, 2025) ;

commit;
 
exit;
EOF
