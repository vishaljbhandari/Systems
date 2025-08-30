sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from DIVIDE_FORMATTER where formatter_name like  '%DIVIDE_1000%'  ;
insert into DIVIDE_FORMATTER values ('DIVIDE_1000',1000) ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
