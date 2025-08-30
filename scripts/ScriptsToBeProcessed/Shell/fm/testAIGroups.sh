$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;

delete from subscr_neural_data_profile ;
delete from subscr_neural_voice_profile ;
delete subscriber where id > 1024 ;
delete account where id > 1024 ;
delete from networks where id = 1 ;

update configurations set value = 1 where config_key in ('AI.ENABLE_CUMULATIVE_VOICE_PROFILE', 'AI.ENABLE_CUMULATIVE_DATA_PROFILE', 'AI.ENABLE_ROC_VOICE_PROFILE', 'AI.ENABLE_ROC_DATA_PROFILE' ) ;

insert into networks (id) values (1) ;

delete from ai_groups ;
insert into ai_groups values ('aigrp1') ;
insert into ai_groups values ('Subscriberless') ;

delete from subscriber_groups where key in ('aigrp1') ;
insert into subscriber_groups values (100, 'aigrp1', 'aigrp1', 10, 3) ;

delete from subscriber_groups where key in ('aigrp2') ;
insert into subscriber_groups values (101, 'aigrp2', 'aigrp2', 11, 3) ;

---- SUBSCRIBER 1
insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code, frd_indicator) values (1025,0,'18101960', to_date('', 'dd/mm/yyyy'),1, 'manu' ,'m' ,'kuman' ,'' ,'' ,'' ,'' ,'', 0) ; 

insert into subscriber(id, subscriber_type, account_id, account_name, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (1025, 0, 1025, '18101960', '+919820535200', sysdate -11,'' ,'' ,'' ,'' ,'' ,'404209820535200' ,'' ,'1' ,'pvn, aigrp1' ,2047, 1,0, 1, 1) ;

---- SUBSCRIBER 2
insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code, frd_indicator) values (1026,0,'18101961', to_date('', 'dd/mm/yyyy'),1, 'manu' ,'m' ,'kuman' ,'' ,'' ,'' ,'' ,'',0) ; 
insert into subscriber(id, subscriber_type, account_id, account_name, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (1026, 0, 1026,'18101961', '+919820535201', sysdate -11,'' ,'' ,'' ,'' ,'' ,'404209820535200' ,'' ,'1' ,'pvn, aigrp2' ,2047, 2,0, 1, 1) ;
--insert into subscriber_group_map values('aigrp2', 1026) ;
								
---- SUBSCRIBER 3
insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code, frd_indicator) values (1027,0,'18101962', to_date('', 'dd/mm/yyyy'),1, 'manu' ,'m' ,'kuman' ,'' ,'' ,'' ,'' ,'',0) ; 

insert into subscriber(id, subscriber_type, account_id, account_name, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (1027, 0, 1027, '18101962', '+919820535202', sysdate -9,'' ,'' ,'' ,'' ,'' ,'404209820535200' ,'' ,'1' ,'pvn, aigrp1' ,2047, 1,0, 1, 1) ;

---- SUBSCRIBER 4
insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code, frd_indicator) values (1028,0,'18101963', to_date('', 'dd/mm/yyyy'),1, 'manu' ,'m' ,'kuman' ,'' ,'' ,'' ,'' ,'',0) ; 
insert into subscriber(id, subscriber_type, account_id, account_name, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (1028, 0, 1028, '18101963', '+919820535212', sysdate -1,'' ,'' ,'' ,'' ,'' ,'404209820535200' ,'' ,'1' ,'pvn, aigrp2' ,2047, 1,0, 1, 1) ;

---- SUBSCRIBER 4
insert into subscriber(id, subscriber_type, account_id, account_name, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (1029, 0, 4, '', '+918845200001', sysdate -1,'' ,'' ,'' ,'' ,'' ,'404209820535200' ,'' ,'1' ,'pvn, Subscriberless' ,2047, 2,0, 1, 1024) ;

commit;
quit;
EOF
if test $? -eq 5
then
	exit 1
fi

exit 0
	
