sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from width_trunc_formatter where formatter_name like  '10TRUNC%'  ;
insert into width_trunc_formatter values ('10TRUNC',10) ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
