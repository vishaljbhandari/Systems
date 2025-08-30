$SQL_COMMAND /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
SET SERVEROUTPUT ON ;

delete from configurations where config_key = 'CLEANUP.RECORDS.CDR.OPTIONS' ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.CDR.OPTIONS', '01/01/1970 00:00:00,30', '0');

update usage_profiles set last_cdr_time_stamp = sysdate-1 ;

---- Called

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
10, 1024, '+919845200005', '+919845200008', null, 1, 60,sysdate
, '404212000008', '504220000008', '404300004', 1, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200008'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
11, 1024, '+919845200005', '+919845200008', null, 1, 60,sysdate
, '404212000008', '504220000008', '404300004', 2, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200008'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
12, 1024, '+919845200005', '+919845200008', null, 1, 60,sysdate
, '404212000008', '504220000008', '404300004', 3, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200008'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
12, 1024, '+919845200005', '+919845200008', null, 1, 60,sysdate
, '404212000008', '504220000008', '404300004', 3, 5200, 10, 5, 1, 1, 1, 0, 2, '+919845200008'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
12, 1024, '+919845200005', '+919845200008', null, 1, 60,sysdate
, '404212000008', '504220000008', '404300004', 3, 5200, 10, 9, 1, 1, 1, 0, 2, '+919845200008'
, 224, '111111'); 

--Caller

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
13, 1024, '+919845200008', '+919845200005', null, 2, 60,sysdate
, '404212000005', '504220000005', '404300004', 1, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
14, 1024, '+919845200008', '+919845200005', null, 2, 60,sysdate
, '404212000005', '504220000005', '404300004', 2, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year, vpmn ) values ( 
15, 1024, '+919845200008', '+919845200005', null, 2, 60,sysdate
, '404212000005', '504220000005', '404300004', 3, 5200, 10, 1, 1, 1, 1, 0, 2, '+919845200005'
, 224, '111111'); 

update cdr set day_of_year = to_number(to_char(time_stamp,'ddd')) ;

commit;

---- GPRS

insert into gprs_cdr (id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
10, 1024, 2,  TO_Date( '07/01/2014 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919845200005'
, '504220000008', '404222000008', '404300004', 1, 1, 'IP', '172.16.2.241', 500, 300
, 128, 5, 5, 50, 'MMS', 5200, 183, 0, 0, '3'); 
commit;
 
insert into gprs_cdr ( id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
11, 1024, 1,  TO_Date( '07/01/2014 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919845200002'
, '504220000005', '404222000005', '404300004', 1, 2, 'IP', '172.16.2.241', 500, 300
, 128, 5, 5, 50, 'DATA', 5200, 183, 0, 0, '3'); 

insert into gprs_cdr ( id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
11, 1024, 1,  TO_Date( '07/01/2014 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919845200002'
, '504220000005', '404222000005', '404300004', 5, 2, 'IP', '172.16.2.241', 200, 200
, 128, 5, 5, 50, 'DATA', 5200, 183, 0, 0, '3'); 

insert into gprs_cdr ( id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
11, 1024, 1,  TO_Date( '07/01/2014 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919845200002'
, '504220000005', '404222000005', '404300004', 9, 2, 'IP', '172.16.2.241', 150, 250
, 128, 5, 5, 50, 'DATA', 5200, 183, 0, 0, '3'); 
commit;

EOF
