sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;
delete from datasource_info where datasource_name like '%PARSER_TEST%' ;
delete from parser where parser_name like 'PARSER%' ;
delete from ascii_delimited_parser_info where parser_name like 'PARSER%'  ;

insert into datasource_info values('PARSER_TEST',2) ; 
insert into parser values('PARSER_TEST',1,'PARSER') ;
insert into ascii_delimited_parser_info values ('PARSER',2048,',','N','Y') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
