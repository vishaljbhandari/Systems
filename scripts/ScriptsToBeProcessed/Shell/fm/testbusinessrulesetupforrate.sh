sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 4;
whenever oserror exit 4;

delete from default_rates where rate_type like 'GS' ;

delete from datasource_info where datasource_name like  '%CMS40_TEST%'  ;
delete from BUSINESSRULE where datasource_name like  '%CMS40%_TEST%'  ;

insert into datasource_info values('CMS40_TEST',1) ;
insert into BUSINESSRULE values ('CMS40_TEST',1,4,'RATE_BR_TEST', 'brrate') ;
insert into default_rates values (default_rates_seq.nextval, 'GS','V',1,500);

delete from RATE_CONFIGURATION where datasource_name like  '%CMS40%_TEST%'  ;
insert into RATE_CONFIGURATION values ('CMS40_TEST','ASN_RECORD_TYPE','ASN_TIME_STAMP','ASN_DURATION','ASN_CALLER_NUMBER',
		'ASN_CALLED_NUMBER','ASN_FORWARDED_NUMBER','ASN_SERVICE_TYPE','ASN_NETWORK','ASN_PHONE_NUMBER','ASN_CALL_TYPE','ASN_VALUE',
		'SUBSCRIBER_ID', 'PMN'); 

delete from datasource_info where datasource_name like  '%MTX_TEST%'  ;
delete from BUSINESSRULE where datasource_name like  '%MTX%_TEST%'  ;

insert into datasource_info values('MTX_TEST',1) ;
insert into BUSINESSRULE values ('MTX_TEST',1,4,'RATE_BR_MTX_TEST', 'brrate') ;

delete from RATE_CONFIGURATION where datasource_name like  '%MTX%_TEST%'  ;
insert into RATE_CONFIGURATION values ('MTX_TEST','RECORD_TYPE','TIME_STAMP','DURATION','CALLER_NUMBER',
		'CALLED_NUMBER','FORWARDED_NUMBER','SERVICE_TYPE','NETWORK','PHONE_NUMBER','CALL_TYPE','VALUE', 'SUBSCRIBER_ID', 'PMN'); 

DELETE FROM default_rates WHERE RATE_TYPE LIKE 'IV' ;
INSERT INTO default_rates VALUES (default_rates_seq.nextval, 'IV', 'V', 1,300) ;
delete from datasource_info where datasource_name like  '%MTX_TEST_INVALID_SUB%'  ;
delete from BUSINESSRULE where datasource_name like  '%MTX_TEST_INVALID_SUB%'  ;

insert into datasource_info values('MTX_TEST_INVALID_SUB',1) ;
insert into BUSINESSRULE values ('MTX_TEST_INVALID_SUB',1,4,'RATE_BR_MTX_TEST_INVALID_SUB', 'brrate') ;

delete from RATE_CONFIGURATION where datasource_name like  '%MTX_TEST_INVALID_SUB%'  ;
insert into RATE_CONFIGURATION values ('MTX_TEST_INVALID_SUB','RECORD_TYPE','TIME_STAMP','DURATION','CALLER_NUMBER',
		'CALLED_NUMBER','FORWARDED_NUMBER','SERVICE_TYPE','NETWORK','PHONE_NUMBER','CALL_TYPE','VALUE', 'SUBSCRIBER_ID', 'PMN'); 

commit;
EOF

if test $? -eq 4
then
	exit 1
fi

exit 0
