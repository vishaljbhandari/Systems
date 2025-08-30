FAILED=3
$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL

whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool statisticalruleexecutorV.dat

select 1 from dual where 1 = ( select count(*) from SCHEDULDED_JOB_EXEC_STATUS where status = $FAILED and job_name='StatisticalRuleExecutor' and start_time is not null and end_time is not null) ;

spool off
EOF

if test $? -eq 5 || grep "no rows" statisticalruleexecutorV.dat >> /dev/null 2>&1
then
	exitval=1
else
	exitval=0
fi

rm -f statisticalruleexecutorV.dat
exit $exitval
