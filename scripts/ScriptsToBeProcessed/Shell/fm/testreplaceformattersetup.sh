sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from replace_formatter where formatter_name like  'REPLACE_FORMATTER_TEST'  ;
insert into replace_formatter values ('REPLACE_FORMATTER_TEST', '-', '') ;
insert into replace_formatter values ('REPLACE_FORMATTER_TEST', 'ab', '999') ;

commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
