sqlplus $DB_USER/$DB_PASSWORD  << EOF
@TestScripts/ranger-db-DSCommon.sql
EOF

if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

exit $exitval

