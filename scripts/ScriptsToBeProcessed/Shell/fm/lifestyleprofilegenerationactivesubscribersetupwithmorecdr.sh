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
delete from frq_called_destinations ;
delete from gprs_life_style_profiles ;
delete from configurations where config_key like 'Top FCD Count' ;

insert into configurations values (configurations_seq.nextval, 'Top FCD Count', '5', '0');

delete from configurations where config_key = 'CLEANUP.RECORDS.CDR.OPTIONS' ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.CDR.OPTIONS', '01/01/1970 00:00:00,30', '0');

---- Called

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
1, 1024, '+919845200005', '+919845200001', null, 1, 40,sysdate-1
, '404212000002', '504220000002', '404300004', 1, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200001'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
3, 1024, '+919845200005', '+919845200001', null, 1, 40, sysdate-1
, '404212000002', '504220000002', '404300004', 2, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200001'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
4, 1024, '+919845200005', '+919845200001', null, 1, 40,sysdate-1
, '404212000002', '504220000002', '404300004', 3, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200001'
, 224, '111111'); 

--Caller

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
3, 1024, '+919845200001', '+919845200005', null, 2, 40,sysdate-1
, '404212000005', '504220000005', '404300004', 1, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
4, 1024, '+919845200001', '+919845200005', null, 2, 40,sysdate-1
, '404212000005', '504220000005', '404300004', 2, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
4, 1024, '+919845200001', '+919845200005', null, 2, 40,sysdate-1
, '404212000005', '504220000005', '404300004', 3, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
4, 1024, '+919845200001', '+919845200005', null, 2, 40,sysdate-1
, '404212000005', '504220000005', '404300004', 3, 5200, 10, 5, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
4, 1024, '+919845200001', '+919845200005', null, 2, 40,sysdate-1
, '404212000005', '504220000005', '404300004', 3, 5200, 10, 9, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 

update cdr set day_of_year = to_number(to_char(time_stamp,'ddd')) ;

commit;

---- GPRS

insert into gprs_cdr (id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1024, 2,  TO_Date( '07/01/2004 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919845200005'
, '504220000001', '404222000001', '404300004', 1, 1, 'IP', '172.16.2.241', 500, 300
, 128, 5, 5, 90, 'MMS', 5200, 183, 0, 0, '3'); 

insert into gprs_cdr (id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1024, 2,  TO_Date( '07/01/2004 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919845200005'
, '504220000001', '404222000001', '404300004', 5, 1, 'IP', '172.16.2.241', 200, 200
, 128, 5, 5, 90, 'MMS', 5200, 183, 0, 0, '3'); 

insert into gprs_cdr (id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1024, 2,  TO_Date( '07/01/2004 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919845200005'
, '504220000001', '404222000001', '404300004', 9, 1, 'IP', '172.16.2.241', 150, 250
, 128, 5, 5, 90, 'MMS', 5200, 183, 0, 0, '3'); 
commit;
 
insert into gprs_cdr ( id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1024, 1,  TO_Date( '07/01/2004 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919845200002'
, '504220000005', '404222000005', '404300004', 1, 2, 'IP', '172.16.2.241', 500, 300
, 128, 5, 5, 90, 'DATA', 5200, 183, 0, 0, '3'); 

commit;

insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code) values (1025, 1, 'A.0000001040', to_date('01/01/2003', 'dd/mm/yyyy'),1024, 'ahmed' ,'abdul kader ali' ,'abdo' ,'aljzair' ,'libia cente' ,'sana''a' ,'sana''a, yemen' ,'1848');

insert into subscriber(id, subscriber_type, account_name, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (5200, 0, 'A.0000001040', 1025, '+919845200005', to_date('01/01/2003', 'dd/mm/yyyy'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025' ,'504220000005' ,'404212000005' ,'1' ,'golden' ,4095, 3,1, 1, 1024) ;

commit;

EOF
