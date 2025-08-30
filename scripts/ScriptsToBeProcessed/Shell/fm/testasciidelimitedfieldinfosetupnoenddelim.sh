sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from ascii_delimited_field_info where parser_name like 'PARSER_NO_DELIM%'  ;
insert into ascii_delimited_field_info values ('PARSER_NO_DELIM',1,'X') ;
insert into ascii_delimited_field_info values ('PARSER_NO_DELIM',2,'Y') ;
insert into ascii_delimited_field_info values ('PARSER_NO_DELIM',3,'Z') ;

commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
