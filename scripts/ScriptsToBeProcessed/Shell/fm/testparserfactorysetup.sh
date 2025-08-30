sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from datasource_info where datasource_name like  'GENERICDS%'  ;
insert into datasource_info values('GENERICDS',1) ;
insert into parser values ('GENERICDS',1,'PARSER_GENERICDS') ;

delete from datasource_info where datasource_name like  '%TILO%'  ;
insert into datasource_info values('TILO',1) ;
insert into parser values ('TILO',0,'PARSER_TILO') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
