sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 4;
whenever oserror exit 4;

delete from LCA where datasource_name like  '%CMS40R10_TEST%'  ;
delete from datasource_info where datasource_name like  '%CMS40R10_TEST%'  ;
delete from BUSINESSRULE where datasource_name like  '%CMS40%_TEST%'  ;

insert into datasource_info values('CMS40R10_TEST',1) ;
insert into BUSINESSRULE values ('CMS40R10_TEST',1,3,'CDR_CALL_TYPE_TEST_BR', 'brcdrcalltype') ;

delete from CALL_TYPE_CONFIGURATION where datasource_name like '%CMS40%_TEST%'  ;

insert into CALL_TYPE_CONFIGURATION values ('CMS40R10_TEST','+1','ASN_CALL_MODULE','ASN_CALL_TYPE','ASN_CALLER_NUMBER','ASN_CALLED_NUMBER','ASN_FORWARDED_NUMBER','MS_ORIGINATING','MS_TERMINATING','CALL_FORWARDING','ROAMING_CALL_FORWARDING') ;

insert into LCA VALUES ('CMS40R10_TEST', 999) ;

delete from LCA where datasource_name like  '%MTX_TEST%'  ;
delete from datasource_info where datasource_name like  '%MTX_TEST%'  ;
delete from BUSINESSRULE where datasource_name like  '%MTX%_TEST%' ;

insert into datasource_info values('MTX_TEST',1) ;
insert into BUSINESSRULE values ('MTX_TEST',1,3,'CDR_CALL_TYPE_TEST_MTX_BR', 'brcdrcalltype') ;

delete from  CALL_TYPE_CONFIGURATION where datasource_name like '%MTX%_TEST%'  ;
insert into CALL_TYPE_CONFIGURATION values ('MTX_TEST','+1','RECORD_TYPE','CALL_TYPE','CALLER_NUMBER','CALLED_NUMBER','FORWARDED_NUMBER','1','2','3','') ;

insert into LCA VALUES ('MTX_TEST', 999) ;

commit;
EOF

if test $? -eq 4
then
	exit 1
fi

exit 0
