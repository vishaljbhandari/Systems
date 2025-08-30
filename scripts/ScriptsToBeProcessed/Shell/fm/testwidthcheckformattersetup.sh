sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from width_check_formatter where formatter_name like  '10CHECK%'  ;
insert into width_check_formatter values ('10CHECK',10) ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
