$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;

delete from subscr_neural_data_profile ;
delete from subscr_neural_voice_profile ;
delete subscriber where id > 1024 ;
delete account where id > 1024 ;
delete from networks where id = 1 ;

update configurations set value = 1 where config_key = 'AI.ENABLE_CUMULATIVE_DATA_PROFILE' ;
update configurations set value = 0 where config_key in ('AI.ENABLE_SINGLE_VOICE_PROFILE', 'AI.ENABLE_CUMULATIVE_VOICE_PROFILE', 'AI.ENABLE_ROC_VOICE_PROFILE', 'AI.ENABLE_ROC_DATA_PROFILE') ;

insert into networks (id) values (1) ;

delete from ai_groups ;
insert into ai_groups values ('aigrp1') ;

delete from subscriber_groups where key in ('aigrp1') ;
insert into subscriber_groups values (100, 'aigrp1', 'aigrp1', 10, 3) ;
---- SUBSCRIBER 1
insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, address_line_2, address_line_3, city, post_code, frd_indicator) values (1025, 0, '18101960', to_date('', 'dd/mm/yyyy'),1, 'manu' ,'m' ,'kuman' ,'' ,'' ,'' ,'' ,'', 0) ;

insert into subscriber(id, subscriber_type, account_id, account_name, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) values (1025, 0, 1025, '18101960', '+919820535200', sysdate -11,'' ,'' ,'' ,'' ,'' ,'404209820535200' ,'' ,'1' ,'pvn, aigrp1' ,2047, 1,0, 1, 1) ;

commit;
quit;
EOF
if test $? -eq 5
then
	exit 1
fi

exit 0
	

