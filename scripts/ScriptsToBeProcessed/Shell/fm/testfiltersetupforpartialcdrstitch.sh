sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 4;
whenever oserror exit 4;

delete from partial_cdr_configuration  where filter_name like  '%PARTIAL_CDR_STITCH_TEST%'  ;
insert into partial_cdr_configuration values ('PARTIAL_CDR_STITCH_TEST', 1, 'ASN_CALL_IDENTIFICATION_NUMBER', 'ASN_DATE_FOR_START_OF_CALL', 'ASN_TIME_FOR_START_OF_CALL', 'ASN_PARTIAL_OUPUT_REC_NUM', 'ASN_LAST_PARTIAL_OUTPUT','ASN_DURATION','ASN_TIME_STAMP','ASN_IS_ATTEMPTED', '%Y/%m/%d %H:%M:%S') ;

commit;
EOF

if test $? -eq 4
then
	exit 1
fi

exit 0
