#! /bin/bash
echo $$ > $RANGERHOME/FMSData/Scheduler/CounterCleanupMultiFieldPid$NetworkID

on_exit ()
{
	rm -f $RANGERHOME/FMSData/Scheduler/CounterCleanupMultiFieldPid$NetworkID
	exit;
}
trap on_exit EXIT 

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL

whenever sqlerror exit 5;
set feedback off;
set serverout on ;

begin
for cleanup_config in (select distinct cleanup_interval_in_days from counter_cleaner_configurations)
	loop
		delete from multiple_field_counter where aggregate_type in (select aggregation_type from
		counter_cleaner_configurations where cleanup_interval_in_days = cleanup_config.cleanup_interval_in_days) and
		record_time <= ((sysdate-cleanup_config.cleanup_interval_in_days)-to_date('01/01/1970','dd/mm/yyyy'))*86400 ;
		commit ;
	end loop ;
end ;
/

Commit ;
QUIT ;
EOF

exitval=$?

if [ $exitval -ne 0 ]
then
    echo "`date`:: Error While Running Counter Cleanup for Counter - Multi Field Counter" >> $RANGERHOME/tmp/CounterCleanup.log
else
    echo "`date`:: Successfully completed Counter Cleanup for Counter - Multi Field Counter" >> $RANGERHOME/tmp/CounterCleanup.log
fi

exit $exitval 
