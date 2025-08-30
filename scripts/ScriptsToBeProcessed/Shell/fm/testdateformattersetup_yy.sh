sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from date_formatter where formatter_name like  'DATE%'  ;
insert into date_formatter values ('DATE','%d%m%y%H%M%S','%d/%m/%Y %H:%M:%S') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
