$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;

delete from cdr;
delete from subscr_neural_data_profile;
delete from subscr_neural_voice_profile;

delete from configurations where config_key = 'AI.CUMREC_VOICE_LAST_PROCESSED_DAY_OF_YEAR_1' ;
delete from configurations where config_key = 'LAST_CDR_TIME_0' ;

insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'AI.CUMREC_VOICE_LAST_PROCESSED_DAY_OF_YEAR_1', '1') ;
insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'LAST_CDR_TIME_0', '10/01/2005 10:00:00,') ;

update configurations set value = 4 where config_key = 'AI.CUMREC_VOICE_LOOKBACK_DAYS' ;

insert into subscr_neural_voice_profile (subscriber_id, phone_number, network_id, sequence_id, profile_type, profile_matured)
	values(1025, '+919820535200', 1, 0, 2, 1) ;

insert into cdr (id,network_id, service_type, cdr_category, is_complete, is_attempted,record_type,time_stamp,
	duration,caller_number,called_number, call_type,subscriber_id,
		value,cdr_type,phone_number,day_of_year, hour_of_day) 
		values(1050,1, 1, 1, 1, 0,1,to_date('06:01:2005 01:00:00','dd:mm:yyyy hh24:mi:ss'),
				230,'+919820535200','+119820535201',2,1025,300,1,
				'+919820535200',6,0) ;

insert into cdr (id,network_id, service_type, cdr_category, is_complete, is_attempted,record_type,time_stamp,
	duration,caller_number,called_number, call_type,subscriber_id,
		value,cdr_type,phone_number,day_of_year, hour_of_day) 
		values(1050,1, 2, 1, 1, 0,1,to_date('07:01:2005 01:00:00','dd:mm:yyyy hh24:mi:ss'),
				0,'+919820535200','+919820535201',1,1025,1,1,
				'+919820535200',7,0) ;


insert into cdr (id,network_id, service_type, cdr_category, is_complete, is_attempted,record_type,time_stamp,
	duration,caller_number,called_number, call_type,subscriber_id,
		value,cdr_type,phone_number,day_of_year, hour_of_day) 
		values(1051,1, 1, 1, 1, 0,1,to_date('09:01:2005 08:30:00','dd:mm:yyyy hh24:mi:ss'),
				230,'+919820535200','+929820535201',3,1025,300,1,
				'+919820535200',9,0) ;

insert into cdr (id,network_id, service_type, cdr_category, is_complete, is_attempted,record_type,time_stamp,
	duration,caller_number,called_number, call_type,subscriber_id,
		value,cdr_type,phone_number,day_of_year, hour_of_day) 
		values(1052,1, 1, 1, 1, 0,1,to_date('10:01:2005 21:00:00','dd:mm:yyyy hh24:mi:ss'),
				230,'+919820535200','+929820535201',3,1025,300,1,
				'+919820535200',10,0) ;

insert into cdr (id,network_id, service_type, cdr_category, is_complete, is_attempted,record_type,time_stamp,
	duration,caller_number,called_number, call_type,subscriber_id,
		value,cdr_type,phone_number,day_of_year, hour_of_day) 
		values(2052,1, 1, 1, 1, 0,1,to_date('05:01:2005 21:00:00','dd:mm:yyyy hh24:mi:ss'),
				230,'+919820535200','+929820535201',3,1025,300,1,
				'+919820535200',5,0) ;


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
	

