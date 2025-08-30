sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from datasource_info where datasource_name like  '%GENERICDS_TEST%'  ;
delete from MANIPULATOR where DATASOURCE_NAME like '%GENERICDS_TEST%' ;
delete from duplicate_field_manip where MANIPULATOR_NAME like '%CUSTOMER_NAME_TEST%' ;

insert into datasource_info values('GENERICDS_TEST',1) ;
insert into manipulator values ('GENERICDS_TEST',1,1,'CUSTOMER_NAME_TEST') ;
insert into duplicate_field_manip values ('CUSTOMER_NAME_TEST', 'CUSTOMER_NAME', 'SUBSCRIBER_NAME') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
