sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from DUPLICATE_CDR_FILE_CONFIG where DS_TYPE like '%GBXUS_CDRDS%' ;

commit;
quit;
EOF
if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

