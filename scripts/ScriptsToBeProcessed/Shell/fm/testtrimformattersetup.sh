sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from trim_formatter where formatter_name like  'TRIM%'  ;
delete from trim_formatter where formatter_name like  'FILLERTRIM%'  ;
insert into trim_formatter values ('FILLERTRIM','F') ;
insert into trim_formatter values ('TRIM',null) ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
