$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL

delete from configurations where config_key = 'LAST_PROCESSED_BLACKLIST_ALARM_ID' ;
delete from subscriber where id > 1024;
delete from cdr;
delete from alert_cdr;
delete from alerts;
delete from alarms;
delete from fraudulent_info ;


whenever sqlerror exit 5;

insert into configurations(id,config_key,value) values(configurations_seq.nextval,'LAST_PROCESSED_BLACKLIST_ALARM_ID', '99999') ;

insert into subscriber(id, account_id,subscriber_type,ACCOUNT_NAME, phone_number, subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id)
values (1025, 1025,1,'AAA','+919820336303', to_date('01/01/2003', 'dd/mm/yyyy'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025',
        '504220000001' ,'404212000002' ,'1' ,'golden' ,4095, 1,1,1,1025) ;

insert into cdr (id,subscriber_id,phone_number,caller_number,called_number,forwarded_number,record_type,duration,time_stamp,equipment_id,imsi_number,geographic_position,call_type,is_attempted, network_id,day_of_year,hour_of_day,value,cdr_type,SERVICE_TYPE,is_complete)
values (1056,1025,'+919820336303','+919820336303','+919820535201','',1,10,sysdate-3,'490519820535201','404209820535201','404300006',1,0,1025,to_char(sysdate - 3.0,'DDD'),to_char(sysdate - 3.0,'HH'),10,1,1,1);

insert into alerts (id, alarm_id, event_instance_id, value, cdr_count, repeat_count, cdr_time, created_date,modified_date, score, aggregation_type, aggregation_value, is_visible)
values(99999, 99999, 99999, 10, 1, 2, sysdate - 20, sysdate - 20, sysdate, 10, 2, '+919820336303', 1);

insert into alert_cdr(ALERT_ID,CDR_ID,CDR_VALUE,RECORD_CATEGORY,OWNER_ID,RUN_ID,AGGREGATION_TYPE,AGGREGATION_VALUE,TIME_STAMP)
	values(99999,1056,10,1,1,alert_cdr_run_id_seq.nextval,2,'+919820336303',sysdate-3) ;

insert into alarms (network_id, id,rule_ids, reference_id, alert_count, created_date, modified_date, status, user_id,score, value, cdr_count, pending_time, reference_type, reference_value, is_visible, case_id,DISPLAY_VALUE)
values (1025, 99999, ' ',1025, 1, sysdate - 20, sysdate - 2, 2, 'radmin', 10, 10, 1, NULL, 1, '+919820336303', 1, 1025,'Prepay Fraud');


commit;
quit;
EOF

exit $?
~
~
~
~
~
~
~
~
~
~
~
~
~
~
~

