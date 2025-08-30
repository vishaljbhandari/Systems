$SQL_COMMAND -s /nolog << EOF > /dev/null
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
set heading off
spool testcleanupRecordsV.dat ;

select 1 from dual where 1 = (select count(*) from cdr) ;
select 1 from dual where 1 = (select count(*) from gprs_cdr) ;
select 1 from dual where 1 = (select count(*) from recharge_log) ;
select 1 from dual where 1058 = (select id from cdr) ;
select 1 from dual where 1062 = (select id from gprs_cdr) ;
select 1 from dual where 1082 = (select id from recharge_log) ;
select 1 from dual where to_char(trunc(sysdate), 'YYYY/MM/DD HH24:MI:SS')||',2' = (select value from configurations where config_key = 'CLEANUP.RECORDS.CDR.OPTIONS') ;
select 1 from dual where to_char(trunc(sysdate), 'YYYY/MM/DD HH24:MI:SS')||',2' = (select value from configurations where config_key = 'CLEANUP.RECORDS.GPRS_CDR.OPTIONS') ;
select 1 from dual where to_char(trunc(sysdate), 'YYYY/MM/DD HH24:MI:SS')||',2' = (select value from configurations where config_key = 'CLEANUP.RECORDS.RECHARGE_LOG.OPTIONS') ;

spool off ;
quit ;
EOF

SqlResult=$?
grep "no rows" testcleanupRecordsV.dat > /dev/null
if [ $? -eq  0 ] || [ $SqlResult -ne  0 ] ; then
	exitval=1
else
	exitval=0
fi
rm -f testcleanupRecordsV.dat
exit $exitval
