$SQL_COMMAND -s /nolog << EOF > /dev/null
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
set serveroutput on ;
spool testcleanupAuditLogV.dat ;

select 1 from dual where 1 = (select count(*) from audit_trails) ;
select 1 from dual where 1 = (select count(*) from audit_files_processed) ;
select 1 from dual where to_char(trunc(sysdate), 'YYYY/MM/DD HH24:MI:SS') = (select to_char(trunc(to_date(value, 'YYYY/MM/DD HH24:MI:SS')),'YYYY/MM/DD HH24:MI:SS') from configurations where config_key = 'CLEANUP.AUDITLOG.LAST_RUN_DATE') ;

spool off ;
quit ;
EOF

SqlResult=$?
grep "no rows" testcleanupAuditLogV.dat > /dev/null
if [ $? -eq  0 ] || [ $SqlResult -ne  0 ] ; then
	exitval=1
else
	exitval=0
fi
rm -f testcleanupAuditLogV.dat
exit $exitval
