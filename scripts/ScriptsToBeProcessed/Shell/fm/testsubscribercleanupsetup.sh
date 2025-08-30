DATE=`date %d/%m/%Y`
$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
delete from subscriber where id > 1024;
delete from account where id > 1024 ;
delete from networks_subscriber_groups where subscriber_group_id=1024 ;
delete from subscriber_groups where id= 1024;
delete from subscr_neural_data_profile ;
delete from subscr_neural_voice_profile ;
delete from blacklist_neural_profile ;
delete from gprs_life_style_profiles ;
delete from usage_profiles ;
delete from FP_PROFILES_16_1 where profiled_entity_id = 1810;
delete from FP_PROFILES_16_2 where profiled_entity_id = 1810;
delete from FP_SUSPECT_PROFILES_16 where profiled_entity_id = 1810;



update configurations set value = '90' where config_key = 'Disconnected Subscriber Cleanup Interval in Days' ;
insert into subscriber_groups (id, key, description,priority, record_config_id) values(1024, 'PVN1', 'Doctor', 1, 1);

insert into account (id, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code) values (1810,'18101960', to_date('', 'dd/mm/yyyy'),1024, 'manu' ,'m' ,'kuman' ,'' ,'' ,'' ,'' ,'') ;
insert into account (id, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code) values (1811,'18101960', to_date('', 'dd/mm/yyyy'),1024, 'manu' ,'m' ,'kuman' ,'' ,'' ,'' ,'' ,'') ;
insert into account (id, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code) values (1812,'18101960', to_date('', 'dd/mm/yyyy'),1024, 'manu' ,'m' ,'kuman' ,'' ,'' ,'' ,'' ,'') ;
insert into account (id, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code) values (1813,'18101960', to_date('', 'dd/mm/yyyy'),1024, 'manu' ,'m' ,'kuman' ,'' ,'' ,'' ,'' ,'') ;

insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number,contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type,	modified_date, network_id) values (1810, 1810, '+919820336300', to_date('', 'dd/mm/yyyy'),'' ,'' ,'' ,'' ,'' ,'404209820535200' ,''	,'1' ,'pvn' ,2047, 3,0, 1,to_date('15/08/2005', 'dd/mm/yyyy'), 1024) ;
insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number,contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type,	modified_date, network_id) values (1811, 1811, '+919820336301', to_date('', 'dd/mm/yyyy'),'' ,'' ,'' ,'' ,'' ,'404209820535200' ,''	,'1' ,'pvn' ,2047, 3,0, 1,to_date('15/08/2005', 'dd/mm/yyyy'), 1024) ;
insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number,contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type,	modified_date, network_id) values (1814, 1811, '+919820336304', to_date('', 'dd/mm/yyyy'),'' ,'' ,'' ,'' ,'' ,'404209820535200' ,''	,'1' ,'pvn' ,2047, 3,0, 1,to_date('$DATE', 'dd/mm/yyyy'), 1024) ;
insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number,contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type,	modified_date, network_id) values (1812, 1812, '+919820336302', to_date('', 'dd/mm/yyyy'),'' ,'' ,'' ,'' ,'' ,'404209820535200' ,''	,'1' ,'pvn' ,2047, 1,0, 1, to_date('15/08/2005', 'dd/mm/yyyy'), 1024) ;
insert into subscriber(id, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number,contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type,	modified_date, network_id) values (1813, 1813, '+919820336303', to_date('', 'dd/mm/yyyy'),'' ,'' ,'' ,'' ,'' ,'404209820535200' ,''	,'1' ,'pvn' ,2047, 1,0, 1,  to_date('15/08/2005', 'dd/mm/yyyy'), 1024) ;

insert into subscr_neural_voice_profile(subscriber_id, phone_number) values (1810, '+919820336300') ;
insert into subscr_neural_voice_profile(subscriber_id, phone_number) values (1811, '+919820336301') ;
insert into subscr_neural_voice_profile(subscriber_id, phone_number) values (1820, '+919820336310') ;

insert into blacklist_neural_profile(phone_number) values ('+919820336300') ;
insert into blacklist_neural_profile(phone_number) values ('+919820336301') ;
insert into blacklist_neural_profile(phone_number) values ('+919820336304') ;
insert into blacklist_neural_profile(phone_number) values ('+919820336303') ;

insert into gprs_life_style_profiles(id, phone_number, subscriber_id, subscriber_type, record_type, profile_type) values (5555, '+919820336300', 1810, 0, 1, 'A');
insert into gprs_life_style_profiles(id, phone_number, subscriber_id, subscriber_type, record_type, profile_type) values (5556, '+919820336301', 1811, 0, 1, 'A');
insert into gprs_life_style_profiles(id, phone_number, subscriber_id, subscriber_type, record_type, profile_type) values (5557, '+919820336310', 1820, 0, 1, 'A');
insert into gprs_life_style_profiles(id, phone_number, subscriber_id, subscriber_type, record_type, profile_type) values (5558, '+919820336302', 1812, 0, 1, 'A');
insert into gprs_life_style_profiles(id, phone_number, subscriber_id, subscriber_type, record_type, profile_type) values (5559, '+919820336303', 1813, 0, 1, 'A');


insert into usage_profiles(id, phone_number, subscriber_id, subscriber_type, record_type, call_type, profile_type) values (5555, '+919820336300', 1810, 0, 1, 1, 'A');
insert into usage_profiles(id, phone_number, subscriber_id, subscriber_type, record_type, call_type, profile_type) values (5556, '+919820336301', 1811, 0, 1, 1, 'A');
insert into usage_profiles(id, phone_number, subscriber_id, subscriber_type, record_type, call_type, profile_type) values (5557, '+919820336310', 1820, 0, 1, 1, 'A');
insert into usage_profiles(id, phone_number, subscriber_id, subscriber_type, record_type, call_type, profile_type) values (5558, '+919820336302', 1812, 0, 1, 1, 'A');
insert into usage_profiles(id, phone_number, subscriber_id, subscriber_type, record_type, call_type, profile_type) values (5559, '+919820336303', 1813, 0, 1, 1, 'A');

INSERT INTO FP_SUSPECT_PROFILES_16 (ID, ENTITY_ID, PROFILED_ENTITY_ID, GENERATED_DATE, SERIAL_NUMBER, ELEMENTS, IS_MATCH_TYPE, VERSION) VALUES (5000, 16, 1810, sysdate, 1, '5000;5001;5002;5003;5004|+919886712342(30);+919884414376(21);+016705195378(4);+912349856784(13)|+916239473334(9);+119356294747(26);+910987778645(41);+117678934566(83)|http://www.mail.yahoo.com/login.html(70);http://wwww.subexa', 1, 0) ;
INSERT INTO FP_SUSPECT_PROFILES_16 (ID, ENTITY_ID, PROFILED_ENTITY_ID, GENERATED_DATE, SERIAL_NUMBER, ELEMENTS, IS_MATCH_TYPE, VERSION) VALUES (5001, 16, 1810, sysdate, 2, 'zure.com/products/nikirav6.2.html(13);http://www.wikipedia.com/en/quantam_mechanics.html(15);http://www.google.co.in(288)|122|5', 1, 0) ;
INSERT INTO FP_SUSPECT_PROFILES_16 (ID, ENTITY_ID, PROFILED_ENTITY_ID, GENERATED_DATE, SERIAL_NUMBER, ELEMENTS, IS_MATCH_TYPE, VERSION) VALUES (5002, 16, 1810, sysdate, 1, '5000;5001;5002;|+919886712342(40);+919884414376(61);+016705195378(8);+912349856784(83)|+916239473334(9);+119356294747(216);+910987778645(241);+117678934566(37)|http://www.mail.cyrex.com/login.html(30);http://www.x.com(9)', 1, 1) ;

commit;
quit;						
EOF
