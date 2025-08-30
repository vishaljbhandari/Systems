$SQL_COMMAND /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
SET SERVEROUTPUT ON ;

delete from cdr ;
delete from gprs_cdr ;
delete from subscriber_profile_dates ;
delete from subscriber where id > 1024 ;
delete from account where id > 1024 ;

delete from usage_profiles ;
delete from frq_called_destinations ;
delete from gprs_life_style_profiles ;


---- GPRS
insert into gprs_cdr (id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1025, 1,  TO_Date( '12/26/2006 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919812400401'
, '355361000284545', '45536100028787', '100200', 1, 1, 'MMS', '172.16.2.241', 500, 300
, 128, 5, 5, 90, 'MMS', 1025, 360, 0, 0, '1'); 
commit;

insert into gprs_cdr (id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1025, 1,  TO_Date( '12/26/2006 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919812400401'
, '355361000284545', '45536100028787', '100200', 1, 2, 'MMS', '172.16.2.241', 500, 300
, 128, 5, 5, 90, 'MMS', 1025, 360, 0, 0, '1'); 
commit;
 
insert into gprs_cdr (id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1025, 2,  TO_Date( '12/26/2006 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919812400401'
, '355361000284545', '45536100028787', '100200', 1, 1, 'MMS', '172.16.2.241', 500, 300
, 128, 5, 5, 90, 'MMS', 1025, 360, 0, 0, '1'); 
commit;

insert into gprs_cdr (id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1025, 2,  TO_Date( '12/26/2006 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919812400401'
, '355361000284545', '45536100028787', '100200', 1, 2, 'MMS', '172.16.2.241', 500, 300
, 128, 5, 5, 90, 'MMS', 1025, 360, 0, 0, '1'); 

insert into gprs_cdr (id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1025, 2,  TO_Date( '12/26/2006 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919812400401'
, '355361000284545', '45536100028787', '100200', 5, 2, 'MMS', '172.16.2.241', 200, 200
, 128, 5, 5, 90, 'MMS', 1025, 360, 0, 0, '1'); 

insert into gprs_cdr (id, network_id, record_type, time_stamp, duration, phone_number, imsi_number,
imei_number, geographic_position, cdr_type, service_type, pdp_type, served_pdp_address,
upload_data_volume, download_data_volume, service, qos_requested, qos_negotiated, value,
access_point_name, subscriber_id, day_of_year, cause_for_session_closing, session_status,
charging_id ) values ( 
1, 1025, 2,  TO_Date( '12/26/2006 01:00:00 AM', 'MM/DD/YYYY HH:MI:SS AM'), 600, '+919812400401'
, '355361000284545', '45536100028787', '100200', 9, 2, 'MMS', '172.16.2.241', 150, 250
, 128, 5, 5, 90, 'MMS', 1025, 360, 0, 0, '1'); 
commit;

insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code) values (1025, 1, '400401', to_date('01/01/2003', 'dd/mm/yyyy'),1025, 'ahmed' ,'abdul kader ali' ,'abdo' ,'aljzair' ,'libia cente' ,'sana''a' ,'sana''a, yemen' ,'1848');

insert into subscriber(id, subscriber_type, account_name, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (1025, 0, '400401', 1025, '+919812400401', to_date('01/01/2003', 'dd/mm/yyyy'), '+918010000101','+918010000101','+918010000101','+918010000101','+918010000101','355361000284545','45536100028787','1','golden',4095, 3,1, 1, 1025) ;

commit;

EOF
