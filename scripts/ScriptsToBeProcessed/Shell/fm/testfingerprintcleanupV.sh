$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool testfingerprintcleanupV.dat

select 1 from dual where 0 = (select count(*) from fp_entityid_16_v where profiled_entity_id = 1810 and entity_id = 16 and version = 0) ; 

spool off
EOF
 
if [ $? -eq '5' ] || grep "no rows" testfingerprintcleanupV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f testfingerprintcleanupV.dat
exit $exitval
