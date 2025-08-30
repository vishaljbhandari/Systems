sqlplus $DB_USER/$DB_PASSWORD<<EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

INSERT INTO ASN1_CALLMODULE_PATH VALUES ('ALLTEL.ROAMEX','AMPS_MTC','1-2') ;

commit ;

EOF

if [ $? -ne 0 ]
then
    exit 1
fi

