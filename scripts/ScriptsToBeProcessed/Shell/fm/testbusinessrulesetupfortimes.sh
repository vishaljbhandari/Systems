sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 4;
whenever oserror exit 4;

delete from datasource_info where datasource_name like  '%TIMES_TEST%'  ;
delete from BUSINESSRULE where datasource_name like  '%TIMES_TEST%'  ;

insert into datasource_info values('TIMES_TEST',1) ;
insert into BUSINESSRULE values ('TIMES_TEST',1,8,'TIMES_TEST_BR', 'brtimes') ;
commit;
EOF

if test $? -eq 4
then
	exit 1
fi

exit 0
