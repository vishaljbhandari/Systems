sqlplus $DB_USER/$DB_PASSWORD<<EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

DELETE FROM ASN1_CALLMODULE_PATH WHERE CALL_MODULE = 'AMPS_MTC' ;
commit ;

EOF

if [ $? -ne 0 ]
then
    exit 1
fi

