$SQL_COMMAND /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
SET SERVEROUTPUT ON ;

delete from cdr;
delete from gprs_cdr ;
delete from subscriber_profile_dates ;
delete from subscriber where id > 1024;
delete from account where id > 1024;

delete from usage_profiles;
delete from frq_called_destinations;
delete from hotlist_groups_suspect_values;
delete from suspect_values;
delete from gprs_life_style_profiles;

delete from blacklist_profile_options;

delete from configurations where config_key in
	('HotList IMSI Expiry in Days','HotList IMEI Expiry in Days',
 		'HotList PhoneNumber Expiry in Days','HotList Cellsite Expiry in Days',
		'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024', 'LAST_PROCESSED_LSP_BLK_ID_FOR_SUB_TYP_1_PROC_TYP_1_1024', 'INSTALLATION.GPRS',
		'Top FCD Count', 'ACA_STOP_PROCESSING_ST_1_PT_1_1024') ;

insert into configurations values (configurations_seq.nextval, 'HotList IMSI Expiry in Days', '30', '0');
insert into configurations values (configurations_seq.nextval, 'HotList IMEI Expiry in Days', '40', '0');
insert into configurations values (configurations_seq.nextval, 'HotList PhoneNumber Expiry in Days', '50', '0');
insert into configurations values (configurations_seq.nextval, 'HotList Cellsite Expiry in Days', '30', '0');
insert into configurations values (configurations_seq.nextval, 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024', '1', '0');
insert into configurations values (configurations_seq.nextval, 'INSTALLATION.GPRS', '1', '0');
insert into configurations values (configurations_seq.nextval, 'Top FCD Count', '5', '0');
insert into configurations values (configurations_seq.nextval, 'ACA_STOP_PROCESSING_ST_1_PT_1_1024', 'true', '0');

---- Called

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
1, 1024, '+919845200002', '+919845200001', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am')
, '404212000002', '504220000002', '404300004', 1, 1025, 10, 1, 1, 1, 1, 0, 2, '+919845200002'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
2, 1024, '+919845200003', '+919845200001', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am')
, '404212000002', '504220000002', '404300004', 1, 4, 10, 8, 1, 1, 1, 0, 2, '+919845200003'
, 224, '111111');
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
3, 1024, '+919845200001', '+919845200002', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am')
, '404212000002', '504220000002', '404300004', 1, 1025, 10, 1, 1, 1, 1, 0, 2, '+919845200002'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
4, 1024, '+919845200001', '+919845200002', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am')
, '404212000002', '504220000002', '404300004', 1, 10252, 10, 1, 1, 1, 1, 0, 2, '+919845200001'
, 224, '111111'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
3, 1024, '+919845200005', '+919845200002', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am')
, '404212000005', '504220000005', '404300004', 1, 10253, 10, 1, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
4, 1024, '+919845200005', '+919845200005', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am')
, '404212000005', '504220000005', '404300004', 1, 10253, 10, 1, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
3, 1024, '+919845200005', '+919845200002', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am')
, '404212000005', '504220000005', '404300004', 1, 10253, 10, 1, 5, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
3, 1024, '+919845200005', '+919845200002', null, 1, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am')
, '404212000005', '504220000005', '404300004', 1, 10253, 10, 1, 9, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 
commit;

---- Caller

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
4, 1024, '+919845220002', '+919845200005', null, 2, 40,  to_date( '08/11/2004 06:00:00 am', 'mm/dd/yyyy hh:mi:ss am')
, '404212000005', '504220000005', '404300005', 1, 10253, 10, 1, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 

---- GPRS

insert into gprs_cdr ( id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1024, 1,  TO_Date( '07/01/2004 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919845200001'
, '504220000001', '404222000001', '404300004', 1, 2, 'IP', '172.16.2.241', 500, 300
, 128, 5, 5, 90, 'DATA', 10252, 183, 0, 0, '3'); 
commit;
 
insert into gprs_cdr ( id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1024, 1,  TO_Date( '07/01/2004 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919845200005'
, '504220000005', '404222000005', '404300004', 1, 2, 'IP', '172.16.2.241', 500, 300
, 128, 5, 5, 90, 'DATA', 10253, 183, 0, 0, '3'); 

insert into gprs_cdr ( id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 1, 1024, 1,  TO_Date( '07/01/2004 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 
600, '+919845200005' , '504220000005', '404222000005', '404300004', 5, 2, 'IP', '172.16.2.241', 
500, 300 , 128, 5, 5, 90, 'DATA', 10253, 183, 0, 0, '3'); 
insert into gprs_cdr ( id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 1, 1024, 1,  TO_Date( '07/01/2004 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 
600, '+919845200005' , '504220000005', '404222000005', '404300004', 9, 2, 'IP', '172.16.2.241', 
500, 300 , 128, 5, 5, 90, 'DATA', 10253, 183, 0, 0, '3'); 
commit;

insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code) values (1025, 1, 'A.0000001040', to_date('01/01/2003', 'dd/mm/yyyy'),1024, 'ahmed' ,'abdul kader ali' ,'abdo' ,'aljzair' ,'libia cente' ,'sana''a' ,'sana''a, yemen' ,'1848');
insert into subscriber(id, subscriber_type, account_name, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (10253, 1, 'A.0000001040', 1025, '+919845200005', to_date('01/01/2003', 'dd/mm/yyyy'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025' ,'504220000005' ,'404212000005' ,'1' ,'golden' ,4095, 3,1, 1, 1024) ;

update configurations set value = '0' where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024';
update configurations set value = '1' where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024';
commit;

/
EOF
