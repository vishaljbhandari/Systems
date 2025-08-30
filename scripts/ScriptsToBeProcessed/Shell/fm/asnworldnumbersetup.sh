$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
SET SERVEROUTPUT ON ;

delete from cdr;
delete from gprs_cdr ;
delete from subscriber_profile_dates ;
delete from subscriber where id > 1024;
delete from account where id > 1024;

delete from blacklist_profile_options ;
delete from audit_trails ;
delete from callto_callby_ph_numbers ;

delete from configurations where config_key = 'CLEANUP.RECORDS.CDR.OPTIONS' ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.CDR.OPTIONS', '01/01/1970 00:00:00,30', '0');

----- Called (Subscriber 5200)

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
	imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
	is_attempted, service, phone_number, day_of_year ) 
values ( 
	1, 1024, '+919845200005', '+919845200001', null, 1, 40,sysdate,
	'404212000002', '504220000002', '404300004', 1, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005' , 224); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
	imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
	is_attempted, service, phone_number, day_of_year ) 
values ( 3, 1024, '+919845200005', '+919845200002', null, 1, 40, sysdate,
	'404212000002', '504220000002', '404300004', 2, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005' , 224); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
	imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
	is_attempted, service, phone_number, day_of_year ) 
values ( 4, 1024, '+919845200005', '+919845200003', null, 1, 40, sysdate,
	'404212000002', '504220000002', '404300004', 3, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005' , 224); 


--Caller (Subscriber 5200)

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
	imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
	is_attempted, service, phone_number, day_of_year ) 
values ( 3, 1024, '+919845200004', '+919845200005', null, 2, 40, sysdate,
	'404212000005', '504220000005', '404300004', 1, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005' , 224); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
	imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
	is_attempted, service, phone_number, day_of_year ) 
values ( 4, 1024, '+919845200006', '+919845200005', null, 2, 40, sysdate,
	'404212000005', '504220000005', '404300004', 2, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005' , 224); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
	imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
	is_attempted, service, phone_number, day_of_year ) 
values ( 4, 1024, '+919845200007', '+919845200005', null, 2, 40, sysdate,
	'404212000005', '504220000005', '404300004', 3, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005' , 224); 

-- Other CDR
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
	imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
	is_attempted, service, phone_number, day_of_year ) 
values ( 4, 1024, '+919845200008', '+919845200006', null, 2, 40, sysdate,
	'404212000005', '504220000006', '404300004', 3, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200006' , 224); 

update cdr set day_of_year = to_number(to_char(time_stamp,'ddd')) ;

commit;


insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code) values (1025, 1, 'A.0000001040', to_date('01/01/2003', 'dd/mm/yyyy'),1024, 'ahmed' ,'abdul kader ali' ,'abdo' ,'aljzair' ,'libia cente' ,'sana''a' ,'sana''a, yemen' ,'1848');

insert into subscriber(id, subscriber_type, account_name, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (5200, 0, 'A.0000001040', 1025, '+919845200005', to_date('01/01/2003', 'dd/mm/yyyy'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025' ,'504220000005' ,'404212000005' ,'1' ,'golden' ,4095, 3,1, 1, 1024) ;

--insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code) values (1026, 1, 'A.0000001041', to_date('01/01/2003', 'dd/mm/yyyy'),1024, 'subex' ,'abdul kader ali' ,'abdo' ,'aljzair' ,'libia cente' ,'sana''a' ,'sana''a, yemen' ,'1848');

--insert into subscriber(id, subscriber_type, account_name, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (5201, 0, 'A.0000001041', 1026, '+919845200006', to_date('01/01/2003', 'dd/mm/yyyy'),'97111026' ,'97121026' ,'97131026' ,'97141026' ,'97151026' ,'504220000006' ,'404212000006' ,'1' ,'golden' ,4095, 3,1, 1, 1024) ;

drop sequence blacklist_profile_options_seq ;
create sequence blacklist_profile_options_seq ;

delete from configurations where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024';
delete from configurations where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024';
delete from configurations where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_1_1024';
delete from configurations where config_key = 'ACA_STOP_PROCESSING_ST_1_PT_1_1024' ;
delete from configurations where config_key = 'ACA_STOP_PROCESSING_ST_0_PT_0_1024' ;
insert into configurations values (configurations_seq.nextval, 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024', '0', 0);
insert into configurations values (configurations_seq.nextval, 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024', '0', 0);
insert into configurations values (configurations_seq.nextval, 'ACA_STOP_PROCESSING_ST_1_PT_1_1024', 'TRUE', 0);
insert into configurations values (configurations_seq.nextval, 'ACA_STOP_PROCESSING_ST_0_PT_0_1024', 'TRUE', 0);
insert into configurations values (configurations_seq.nextval, 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_1_1024', '0', 0);

insert into blacklist_profile_options(id, subscriber_id, phone_number, option_id, network_id)
			select BLACKLIST_PROFILE_OPTIONS_SEQ.nextval, id, PHONE_NUMBER, 2, network_id from subscriber 
				where subscriber_type = 0 and id > 1024;

commit;

EOF
