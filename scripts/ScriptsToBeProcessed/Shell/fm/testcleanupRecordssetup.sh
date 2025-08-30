$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from cdr ;
delete from gprs_cdr ;
delete from recharge_log ;

delete from configurations where config_key='CLEANUP.RECORDS.OPTION' ;
delete from configurations where config_key='CLEANUP.RECORDS.CDR.OPTIONS' ;
delete from configurations where config_key='CLEANUP.RECORDS.GPRS_CDR.OPTIONS' ;
delete from configurations where config_key='CLEANUP.RECORDS.RECHARGE_LOG.OPTIONS' ;

insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.OPTION', '1,2,7', 0) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.CDR.OPTIONS', to_char(trunc(sysdate),'YYYY/MM/DD HH24:MI:SS')||',2', 0) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.GPRS_CDR.OPTIONS', to_char(trunc(sysdate ),'YYYY/MM/DD HH24:MI:SS')||',2', 0) ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.RECHARGE_LOG.OPTIONS', to_char(trunc(sysdate ),'YYYY/MM/DD HH24:MI:SS')||',2', 0) ;

update configurations set value='' where config_key='CLEANUP.RECORDS.LAST_CDR_DAY_TRUNCED';
update configurations set value='' where config_key='CLEANUP.RECORDS.LAST_GPRS_CDR_DAY_TRUNCED';
update configurations set value='' where config_key='CLEANUP.RECORDS.LAST_RECHARGE_LOG_DAY_TRUNCED';

insert into cdr (id,subscriber_id,phone_number,caller_number,called_number,forwarded_number,record_type,duration,
	time_stamp,equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete,
	is_attempted, network_id,day_of_year,hour_of_day)
values (1056,1813,'+919820336303','+919820535200','+919820535201','',1,10,sysdate - 3.0,
	'490519820535201', '404209820535201','404300006',1,10,1,1,1,0,0,to_char(sysdate - 3.0,'DDD'),to_char(sysdate - 3.0,'HH'));
insert into cdr (id,subscriber_id,phone_number,caller_number,called_number,forwarded_number,record_type,duration,
	time_stamp,equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete,
	is_attempted, network_id,day_of_year,hour_of_day)
values (1057,1813,'+919820336303','+919820535200','+919820535202','',1,10,sysdate - 3.02,
	'490519820535202','404209820535202','404300006',1,10,1,1,1,0,0,to_char(sysdate - 3.0,'DDD'),to_char(sysdate - 3.0,'HH'));

insert into gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type,
	upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id,
	imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year)
values(1060,1024,1,sysdate-2,20,'+919820336303',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa',
	'333',to_char(sysdate-2, 'ddd'));
insert into gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type,
	upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id,
	imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year)
values(1061,1024,1,sysdate-2,20,'+919820336303',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa',
	'333',to_char(sysdate-2, 'ddd'));

insert into recharge_log (network_id, id, time_stamp, phone_number, amount, recharge_type,
		status, credit_card, pin_number, serial_number,subscriber_id, account_id)
values (1024, 1080, sysdate-3, '+919820336303', 100, 1, 1, 'credit-card-1025', 'pin-no-1025', 
		'serial-no-1025',2837, 1816) ;
insert into recharge_log (network_id, id, time_stamp, phone_number, amount, recharge_type,
		status, credit_card, pin_number, serial_number,subscriber_id, account_id)
values (1024, 1081, sysdate-2, '+919820336304', 100, 1, 1, 'credit-card-1026', 'pin-no-1026', 
		'serial-no-1027',2839, 1817) ;

--Non Participating Records--
insert into cdr (id,subscriber_id,caller_number,called_number,forwarded_number,record_type,duration,time_stamp,
	equipment_id,imsi_number,geographic_position,call_type,value,cdr_type, service_type, is_complete,
	is_attempted, network_id,day_of_year,hour_of_day)
values (1058,1813,'+919820535200','+919820535202','',1,10,sysdate,'490519820535202',
	'404209820535202','404300006',1,10,1,1,1,0,0,to_char(sysdate,'DDD'),to_char(sysdate + 90,'HH')) ;
insert into gprs_cdr(id, network_id, record_type, time_stamp, duration, phone_number,cdr_type, service_type,
	upload_data_volume,download_data_volume, qos_requested, qos_negotiated, value,subscriber_id ,charging_id,
	imsi_number,imei_number,geographic_position,pdp_type,served_pdp_address,access_point_name,day_of_year)
values(1062,1024,1,sysdate,20,'+919820336308',1,1,12,12,2,2,2,1819,'22','22','222','222','IP','aaa.aaa',
	'333',to_char(sysdate, 'ddd')) ;
insert into recharge_log (network_id, id, time_stamp, phone_number, amount, recharge_type,
		status, credit_card, pin_number, serial_number,subscriber_id, account_id)
values (1024, 1082, sysdate, '+919820336302', 100, 1, 1, 'credit-card-1027', 'pin-no-1027',
		'serial-no-1028',2838, 1818) ;

update cdr set day_of_year = to_number(to_char(time_stamp, 'DDD')) ;
update cdr set hour_of_day = to_number(to_char(time_stamp, 'HH')) ;

update gprs_cdr set day_of_year = to_number(to_char(time_stamp, 'DDD')) ;
update gprs_cdr set hour_of_day = to_number(to_char(time_stamp, 'HH')) ;

update recharge_log set day_of_year = to_number(to_char(time_stamp, 'DDD')) ;
update recharge_log set hour_of_day = to_number(to_char(time_stamp, 'HH')) ;

commit;
EOF

if [ $? -ne 0 ];then
	exit 1
fi
