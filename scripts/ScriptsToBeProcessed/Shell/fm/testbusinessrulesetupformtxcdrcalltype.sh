sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 4;
whenever oserror exit 4;

delete from LCA where datasource_name like  '%MTX_TEST%'  ;
delete from BUSINESSRULE where datasource_name like  '%MTX%_TEST%'  ;
insert into BUSINESSRULE values ('MTX_TEST',1,11,'CDR_CALL_TYPE_TEST_BR', 'brinvalid') ;

delete from  CALL_TYPE_CONFIGURATION where datasource_name like '%MTX%_TEST%'  ;
insert into CALL_TYPE_CONFIGURATION values ('MTX_TEST','+1','RECORD_TYPE','CALL_TYPE') ;

insert into LCA VALUES ('MTX_TEST', 999) ;
commit;
EOF

if test $? -eq 4
then
	exit 1
fi

exit 0
