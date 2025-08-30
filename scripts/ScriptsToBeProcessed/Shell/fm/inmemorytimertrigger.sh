#! /bin/bash

. rangerenv.sh
if [ $#  -ne 5 ]
then
	echo "Usage :./inmemorytimertrigger.sh <CounterID> <CounterInstanceID> <SlaveID> <TimerType[1]> <RecordCategory>"
	echo "TimerType => IM_COUNTER_SWEEP_TIMER = 1 "
	echo "RecordCategory => 0 for Sweep Timer"
	exit 1
fi

SIGHUP=1
TIMER_ELAPSED=4
ReloadCode=30

CounterID=$1
CounterInstance=$2
PartitionID=$3

#*********TimerType*********
#IM_COUNTER_SWEEP_TIMER=1
#UNKNOWN_TIMER_TYPE=2
#***************************

TimerType=$4
RecordCategory=$5

RELOAD_ENTITY_ID="$CounterID,$CounterInstance,$PartitionID,$TimerType,$RecordCategory"

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 6;
set heading off;
set feedback off ;

INSERT INTO reload_configurations (id, reload_key, reload_code, reload_type, reload_entity_id) VALUES
				(reload_configurations_seq.nextval,'PROGRAM_MANAGER.RELOAD.CODE'||$ReloadCode, $ReloadCode, $TIMER_ELAPSED, '$RELOAD_ENTITY_ID') ; 
commit ; 
quit;
EOF

sqlretcode=`echo $?`
if [ $sqlretcode -ne 0 ]
then
	echo "Failed to insert reload entry"
exit 1;
fi

$RANGERHOME/sbin/scriptlauncher -R
echo
echo "Sent ReloadSignal to Program Manager."
