sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from datasource_info where datasource_name like  '%GENERICDS_TEST%'  ;

insert into datasource_info values('GENERICDS_TEST',1) ;
insert into manipulator values ('GENERICDS_TEST',1,1,'DUP_NAME_TEST1') ;
insert into manipulator values ('GENERICDS_TEST',2,1,'DUP_NAME_TEST2') ;

delete from datasource_info  where datasource_name like  'IGENERICDS%'  ;
insert into datasource_info values('IGENERICDS',1) ;
insert into manipulator values ('IGENERICDS',1,77,'INVALID_MANIP') ;


commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
