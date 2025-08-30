$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool testHotlistCleanupSetupV.dat

select 1 from dual where 1 = (select count(*) from suspect_values);

spool off
EOF

if [ $? -ne  '0' ] || grep "no rows" testHotlistCleanupSetupV.dat
then
	exitval=1
else
	exitval=0
fi 
rm -f testHotlistCleanupSetupV.dat
exit $exitval
