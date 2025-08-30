sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 4;
whenever oserror exit 4;

delete from ROAM_LINE_RANGE where datasource_name like  '%CMS40R10_TEST%' ;
delete from datasource_info where datasource_name like  '%CMS40R10_TEST%'  ;

insert into datasource_info values('CMS40R10_TEST',1) ;
insert into ROAM_LINE_RANGE values ('CMS40R10_TEST','9999999999', '99999999999999') ;

delete from BUSINESSRULE where datasource_name like  '%CMS40%_TEST%'  ;
insert into BUSINESSRULE values ('CMS40R10_TEST',1,6,'SUBSCRIBER_ID_TEST_BR', 'brsubscriberid') ;

delete from SUBSCRIBER_ID_CONFIGURATION where datasource_name like  '%CMS40%_TEST%'  ;
insert into SUBSCRIBER_ID_CONFIGURATION values('CMS40R10_TEST','ASN_IMSI','ASN_PHONE_NUMBER','ASN_SUBSCRIBER_ID') ;

delete from datasource_info where datasource_name like  '%MTX_TEST%'  ;
insert into datasource_info values('MTX_TEST',1) ;

delete from ROAM_LINE_RANGE where datasource_name like  '%MTX_TEST%' ;
insert into ROAM_LINE_RANGE values ('MTX_TEST','9999999999', '99999999999999') ;

delete from BUSINESSRULE where datasource_name like  '%MTX%_TEST%'  ;
insert into BUSINESSRULE values ('MTX_TEST',1,6,'SUBSCRIBER_ID_TEST_MTX_BR', 'brsubscriberid') ;

delete from SUBSCRIBER_ID_CONFIGURATION where datasource_name like  '%MTX%_TEST%'  ;
insert into SUBSCRIBER_ID_CONFIGURATION values('MTX_TEST','IMSI','PHONE_NUMBER','SUBSCRIBER_ID') ;


commit;
EOF

if test $? -eq 4
then
	exit 1
fi

exit 0
