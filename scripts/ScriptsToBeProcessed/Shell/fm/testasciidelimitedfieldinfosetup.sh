sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from ascii_delimited_field_info where parser_name like 'PARSER%'  ;
insert into ascii_delimited_field_info values ('PARSER',1,'X') ;
insert into ascii_delimited_field_info values ('PARSER',2,'Y') ;
insert into ascii_delimited_field_info values ('PARSER',3,'Z') ;

commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
