sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;
delete from datasource_info where datasource_name like '%PARSER_NO_DELIM_TEST%' ;
delete from parser where parser_name like 'PARSER_NO_DELIM%' ;
delete from ascii_delimited_parser_info where parser_name like 'PARSER_NO_DELIM%'  ;

insert into datasource_info values('PARSER_NO_DELIM_TEST',2) ; 
insert into parser values('PARSER_NO_DELIM_TEST',1,'PARSER_NO_DELIM') ;
insert into ascii_delimited_parser_info values ('PARSER_NO_DELIM',2048,',','N','N') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
