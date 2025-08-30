sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from ascii_delimited_field_info where parser_name like 'PARSER%'  ;
delete from ascii_delimited_parser_info where parser_name like 'PARSER%'  ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
