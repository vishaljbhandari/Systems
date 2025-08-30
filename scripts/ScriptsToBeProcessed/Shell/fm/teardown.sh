sqlplus $DB_USER/$DB_PASSWORD << X
whenever sqlerror exit 5;
whenever oserror exit 5;

set heading off;

drop table ds_configuration ;

commit;
quit;

X
if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi
exit $exitval
