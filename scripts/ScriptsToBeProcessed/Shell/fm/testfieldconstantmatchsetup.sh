sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from MATCH_FIELD_CONSTANT_FILTER where FILTER_NAME like '%MATCH_CALL_MODULE_TEST%' ;
insert into MATCH_FIELD_CONSTANT_FILTER values ('MATCH_CALL_MODULE_TEST', 'ASN_CALL_MODULE', 'TRANSIT','Y') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
