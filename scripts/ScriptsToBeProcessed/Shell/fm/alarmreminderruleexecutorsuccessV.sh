SUCCESS=2
$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL

whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool alarmreminderruleexecutorV.dat

select 1 from dual where 1 = ( select count(*) from SCHEDULDED_JOB_EXEC_STATUS where status = $SUCCESS and
job_name='AlarmReminderRuleExecutor' and start_time is not null and end_time is not null) ;

spool off
EOF

if test $? -eq 5 || grep "no rows" alarmreminderruleexecutorV.dat >> /dev/null 2>&1
then
	exitval=1
else
	exitval=0
fi

rm -f alarmreminderruleexecutorV.dat
exit $exitval
