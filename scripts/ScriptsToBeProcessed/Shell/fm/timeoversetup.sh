sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from configuration where id = 'PartialCDRTimeOver' ; 
insert into configuration (id, value) values ('PartialCDRTimeOver' , 5) ;
commit;
EOF
if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

exit $exitval
