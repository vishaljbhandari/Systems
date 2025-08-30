sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from prefix_filter where filter_name like 'PREFIX_200_TEST%' ;
insert into prefix_filter values ('PREFIX_200_TEST', 'CALLER_NUMBER','200','Y') ;

commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
