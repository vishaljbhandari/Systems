sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from MATCH_FIELD_FILTER where FILTER_NAME like '%MATCH_CALLED_NUM_TEST%' ;
insert into MATCH_FIELD_FILTER values ('MATCH_CALLED_NUM_TEST', 'CALLER_NUMBER', 'CALLED_NUMBER','Y') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
