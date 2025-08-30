$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
set heading off
spool testalarmcleanupV.dat

select 1 from dual where 1 = (select count(*) from alarm_cleanup) ;
select 1 from dual where 99999 = (select id from alarm_cleanup) ;
select 1 from dual where 0 = (select count(*) from alarms) ;
select 1 from dual where 0 = (select count(*) from alerts) ;
select 1 from dual where 1 = (select count(*) from case) ;
select 1 from dual where 1025 = (select id from case) ;
select 1 from dual where 0 = (select count(*) from matched_results) ;
select 1 from dual where to_char(trunc(sysdate), 'YYYY/MM/DD HH24:MI:SS') = (select to_char(trunc(to_date(value, 'YYYY/MM/DD HH24:MI:SS')),'YYYY/MM/DD HH24:MI:SS') from configurations where config_key = 'CLEANUP.ALARMS.LAST_RUN_DATE') ;

spool off
EOF

SqlResult=$?

ls $COMMON_MOUNT_POINT/Attachments/Alarm/99999*
FileResult=$?
ls $COMMON_MOUNT_POINT/Attachments/Case/99999*
FileResult=`expr $FileResult + $?`

if [ $SqlResult -ne  0 ] || [ $FileResult -eq  0 ] || grep "no rows" testalarmcleanupV.dat
then
	exitval=1
else
	exitval=0
fi

rm -f testalarmcleanupV.dat
exit $exitval
