sqlplus $DB_USER/$DB_PASSWORD << EOF

whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

delete from RANGE_VALIDATOR where VALIDATOR_NAME like  'TESTRANGE%' ;
insert into RANGE_VALIDATOR values ('TESTRANGE', '1', '100',1) ;
commit ;

EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
