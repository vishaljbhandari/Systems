#! /bin/bash
SummaryDate=$1
DayOfYear=$2
HourOfDay=$3

on_exit ()
{
	rm -f $RANGERHOME/FMSData/Scheduler/SwitchTrunkSummaryPid
}
trap on_exit EXIT 

. $RANGERHOME/sbin/rangerenv.sh

if [ $# -lt 3 ];then
	echo "Usage: $0 SummaryDate<dd-mm-yyyy> DayOfYear HourOfDay" ;
	exit 1 ;
fi

sqlplus -s /nolog << EOF
CONNECT $DB_USER/$DB_PASSWORD
WHENEVER SQLERROR EXIT 5 ;
SET FEEDBACK OFF ;
SET SERVEROUTPUT ON ;

exec SwitchTrunkSummary('$SummaryDate', $DayOfYear, $HourOfDay);

quit ;

EOF

