$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;

delete from gprs_cdr;
delete from subscr_neural_data_profile;
delete from subscr_neural_voice_profile;

delete from configurations where config_key = 'AI.CUMREC_DATA_LAST_PROCESSED_DAY_OF_YEAR_1' ;
delete from configurations where config_key = 'LAST_GPRS_CDR_TIME0' ;

insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'AI.CUMREC_DATA_LAST_PROCESSED_DAY_OF_YEAR_1', '1') ;
insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'LAST_GPRS_CDR_TIME0', '10/01/2005 10:00:00,') ;

update configurations set value = 4 where config_key = 'AI.CUMREC_DATA_LOOKBACK_DAYS' ;
	
insert into subscr_neural_data_profile (subscriber_id, phone_number, network_id, sequence_id, profile_type, profile_matured)
	values(1025, '+919820535200', 1, 0, 4, 1) ;

insert into gprs_cdr
	(ID, NETWORK_ID, RECORD_TYPE, TIME_STAMP, DURATION, PHONE_NUMBER,CDR_TYPE, SERVICE_TYPE,UPLOAD_DATA_VOLUME,DOWNLOAD_DATA_VOLUME, QOS_REQUESTED, 
	QOS_NEGOTIATED, VALUE,SUBSCRIBER_ID ,CHARGING_ID,IMSI_NUMBER,IMEI_NUMBER,GEOGRAPHIC_POSITION,PDP_TYPE,SERVED_PDP_ADDRESS, ACCESS_POINT_NAME, day_of_year, hour_of_day)
	values(1050,1,1,to_date('06:01:2005 01:00:00','dd:mm:yyyy hh24:mi:ss'),230,'+919820535200', 1,1,15,16,2,2,300,1025,'22','22','222','222',
				'IP','aaa.aaa','333', 6, 0);

insert into gprs_cdr
	(ID, NETWORK_ID, RECORD_TYPE, TIME_STAMP, DURATION, PHONE_NUMBER,CDR_TYPE, SERVICE_TYPE,UPLOAD_DATA_VOLUME,DOWNLOAD_DATA_VOLUME, QOS_REQUESTED, 
	QOS_NEGOTIATED, VALUE,SUBSCRIBER_ID ,CHARGING_ID,IMSI_NUMBER,IMEI_NUMBER,GEOGRAPHIC_POSITION,PDP_TYPE,SERVED_PDP_ADDRESS, ACCESS_POINT_NAME, day_of_year, hour_of_day)
	values(1051,1,1,to_date('07:01:2005 01:00:00','dd:mm:yyyy hh24:mi:ss'),0,'+919820535200', 1,2,15,17,2,2,1,1025,'22','22','222','222',
				'IP','aaa.aaa','333', 7, 0);

insert into gprs_cdr
	(ID, NETWORK_ID, RECORD_TYPE, TIME_STAMP, DURATION, PHONE_NUMBER,CDR_TYPE, SERVICE_TYPE,UPLOAD_DATA_VOLUME,DOWNLOAD_DATA_VOLUME, QOS_REQUESTED, 
	QOS_NEGOTIATED, VALUE,SUBSCRIBER_ID ,CHARGING_ID,IMSI_NUMBER,IMEI_NUMBER,GEOGRAPHIC_POSITION,PDP_TYPE,SERVED_PDP_ADDRESS, ACCESS_POINT_NAME, day_of_year, hour_of_day)
	values(1052,1,1,to_date('09:01:2005 08:30:00','dd:mm:yyyy hh24:mi:ss'),230,'+919820535200', 1,1,15,18,2,2,300,1025,'22','22','222','222',
				'IP','aaa.aaa','333', 9, 0);

insert into gprs_cdr
	(ID, NETWORK_ID, RECORD_TYPE, TIME_STAMP, DURATION, PHONE_NUMBER,CDR_TYPE, SERVICE_TYPE,UPLOAD_DATA_VOLUME,DOWNLOAD_DATA_VOLUME, QOS_REQUESTED, 
	QOS_NEGOTIATED, VALUE,SUBSCRIBER_ID ,CHARGING_ID,IMSI_NUMBER,IMEI_NUMBER,GEOGRAPHIC_POSITION,PDP_TYPE,SERVED_PDP_ADDRESS, ACCESS_POINT_NAME, day_of_year, hour_of_day)
	values(1053,1,1,to_date('10:01:2005 21:00:00','dd:mm:yyyy hh24:mi:ss'),230,'+919820535200', 1,1,15,15,2,2,300,1025,'22','22','222','222',
				'IP','aaa.aaa','333', 10, 0);

insert into gprs_cdr
	(ID, NETWORK_ID, RECORD_TYPE, TIME_STAMP, DURATION, PHONE_NUMBER,CDR_TYPE, SERVICE_TYPE,UPLOAD_DATA_VOLUME,DOWNLOAD_DATA_VOLUME, QOS_REQUESTED, 
	QOS_NEGOTIATED, VALUE,SUBSCRIBER_ID ,CHARGING_ID,IMSI_NUMBER,IMEI_NUMBER,GEOGRAPHIC_POSITION,PDP_TYPE,SERVED_PDP_ADDRESS, ACCESS_POINT_NAME, day_of_year, hour_of_day)
	values(1054,1,1,to_date('05:01:2005 21:00:00','dd:mm:yyyy hh24:mi:ss'),230,'+919820535200', 1,1,15,15,2,2,300,1025,'22','22','222','222',
				'IP','aaa.aaa','333', 5, 0);


commit;
quit;
EOF
if test $? -eq 5
then
	exitval=1
else
	exitval=0
fi

exit $exitval
	

