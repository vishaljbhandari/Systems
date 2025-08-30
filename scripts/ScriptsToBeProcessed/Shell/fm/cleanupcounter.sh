#! /bin/bash
CounterDetailId=$2
NetworkID=$1
echo $$ > $RANGERHOME/FMSData/Scheduler/CounterCleanupPid$NetworkID

on_exit ()
{
	rm -f $RANGERHOME/FMSData/Scheduler/CounterCleanupPid$NetworkID
	exit;
}
trap on_exit EXIT 

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL

whenever sqlerror exit 5;
set feedback off;

insert into CLEANUP_CONFIGURATIONS values (counter_cleaner_config_seq.nextval, 'PROGRAM_MANAGER.CLEANUP.CODE' ,1 , 3 , $CounterDetailId) ;  

Commit ;
QUIT ;
EOF

$RANGERHOME/sbin/scriptlauncher -R
echo "Sent ReloadSignal to Program Manager."

exitval=$?

if [ $exitval -ne 0 ]
then
    echo "`date`:: Error While Running Counter Cleanup for Counter - $CounterDetailId" >> $RANGERHOME/tmp/CounterCleanup.log
else
    echo "`date`:: Successfully initiated Counter Cleanup for Counter - $CounterDetailId" >> $RANGERHOME/tmp/CounterCleanup.log
fi

exit $exitval 
