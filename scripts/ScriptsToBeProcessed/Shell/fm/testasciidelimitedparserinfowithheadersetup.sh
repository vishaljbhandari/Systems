sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;
delete from datasource_info where datasource_name like '%PARSER_TEST_HEADER%' ;
delete from parser where parser_name like 'PARSER2%' ;
delete from ascii_delimited_parser_info where parser_name like 'PARSER2%'  ;

insert into datasource_info values('PARSER_TEST_HEADER',2) ; 
insert into parser values('PARSER_TEST_HEADER',1,'PARSER2') ;
insert into ascii_delimited_parser_info values ('PARSER2',2048,',','Y','Y') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
