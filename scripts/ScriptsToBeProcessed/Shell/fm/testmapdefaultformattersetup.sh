sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from map_formatter where formatter_name like  'MAP%'  ;
insert into map_formatter values ('MAP','F','G') ;
insert into map_formatter values ('MAP','H','I') ;
insert into map_formatter values ('MAP','DEFAULT','DEF') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
