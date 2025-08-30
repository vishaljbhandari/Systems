sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from ds_field_config where ds_type = 1 ;
insert into ds_field_config values('Receive Time',4,1,1,1,'','yyyy/mm/dd hh24:mi:ss','',1,1);
insert into ds_field_config values('Network',1,2,2,1,'NETWORK_ID','','',1,1);
insert into ds_field_config values('Caller Number',3,3,3,1,'CALLER_NUMBER','','',1,1);
insert into ds_field_config values('Called Number',3,4,4,1,'CALLED_NUMBER','','',1,1);
insert into ds_field_config values('Forwarded Number',3,5,5,1,'FORWARDED_NUMBER','','',1,1);
insert into ds_field_config values('Record Type',1,6,6,1,'RECORD_TYPE','','',1,1);
insert into ds_field_config values('Duration',1,7,7,1,'DURATION','','',1,1);
insert into ds_field_config values('Time Stamp',4,8,8,1,'TIME_STAMP','yyyy/mm/dd hh24:mi:ss','',1,1);
insert into ds_field_config values('Equipment ID',3,9,9,1,'EQUIPMENT_ID','','',1,1);
insert into ds_field_config values('IMSI/ESN Number',3,10,10,1,'IMSI_NUMBER','','',1,1); 
insert into ds_field_config values('Geographic Position',3,11,11,1,'GEOGRAPHIC_POSITION','','',1,1);
insert into ds_field_config values('Call Type',1,12,12,1,'CALL_TYPE','','',1,1);
insert into ds_field_config values('Subscriber ID',1,13,13,1,'SUBSCRIBER_ID','','',0,1);
insert into ds_field_config values('Value',2,14,14,1,'VALUE','','',1,1);
insert into ds_field_config values('CDR Type',1,15,15,1,'CDR_TYPE','','',1,1);
insert into ds_field_config values('Service Type',1,16,16,1,'SERVICE_TYPE','','',1,1);
insert into ds_field_config values('CDR Category',1,17,17,1,'CDR_CATEGORY','','',1,1);
insert into ds_field_config values('Is Complete',1,18,18,1,'IS_COMPLETE','','',1,1);
insert into ds_field_config values('Co-Related Field',3,19,19,1,'','','',1,1);
insert into ds_field_config values('Is Attempted',1,20,20,1,'IS_ATTEMPTED','','',1,1);
insert into ds_field_config values('Service',1,21,21,1,'SERVICE','','', 1,1);
insert into ds_field_config values('Phone Number',3,22,22,1,'PHONE_NUMBER','','',1,1);
commit;
EOF
if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

exit $exitval
