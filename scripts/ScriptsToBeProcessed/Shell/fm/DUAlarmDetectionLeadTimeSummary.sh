#################################################################################
#  Copyright (c) Subex Systems Limited 2007. All rights reserved.               #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Systems Limited. The program (s) may be used and/or copied with the written  #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

#!/bin/bash

NetworkID=$1
DateToSummarize=$2

. rangerenv.sh

if [ -z "$NetworkID" ]; then
	NetworkID="999"
fi

if [ -z "$DateToSummarize" ]; then
	DateToSummarize=`sqlplus -s /nolog << EOF
		CONNECT_TO_SQL
		set heading off
		set feedback off
		set pages 0
		select to_char (sysdate - 1, 'dd/mm/yyyy') from dual ;
EOF`
		DateToSummarize=`echo $DateToSummarize | cut -d. -f2`
fi

on_exit ()
{
	rm -f $RANGERHOME/RangerData/Scheduler/DUAlarmDetectionLeadTimeSummary$NetworkID
}
trap on_exit EXIT

UpdateAlarmDetectionSummary()
{
	sqlplus -s /nolog << EOF > $RANGERHOME/tmp/DUAlarmDetectionLeadTimeSummary.log
	CONNECT_TO_SQL
		whenever sqlerror exit 5 ;
		whenever oserror exit 6 ;
		delete from du_alarm_detection_summary 
			where 
				trunc(alarm_created_date) = to_date('$DateToSummarize', 'dd/mm/yyyy')
				or trunc(alarm_created_date) <= to_date('$DateToSummarize', 'dd/mm/yyyy') - 
					(select value from configuration where id like 'Alarm Cleanup-Interval in Days%') ;

		insert into du_alarm_detection_summary
			(event_name,alarm_created_date,detection_time) 
			select 
				distinct c.name, trunc(a.created_date), trunc(b.created_date) - get_date_diff (trunc(b.created_date), b.start_day)
			from
				alarms a, alerts b, (select key, name, accumulation_function from rules) c
			where trunc(a.created_date) = to_date('$DateToSummarize', 'dd/mm/yyyy')
				and a.id = b.alarm_id
				and b.event_instance_id = c.key
				and a.status in (2, 4)
				and a.is_visible = 1
				and c.accumulation_function not in(13,16,18,19)
				and c.name != 'CRMI_Instance' 
				and c.name != 'CRMIReplicateTroubleTicket'
				and c.name != 'PGWInstance'
				and c.name != 'CRMICreateAlarm' ;
		quit ;
EOF
	return $?
}

echo "     *************** DU Alarm Detection Lead Time Summary ***************" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
echo -n "  Start Date : `date +%Y/%m/%d`          " >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
echo "  Start Time : `date +%T`" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log

UpdateAlarmDetectionSummary

ReturnCode=$?

echo -n "  End Date   : `date +%Y/%m/%d`          " >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
echo "  End Time   : `date +%T`" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
echo "     --------------------------------------------------------------------" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
echo "  Summarization Run For Date : $DateToSummarize" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
echo "  Network ID                 : $NetworkID" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
echo "     --------------------------------------------------------------------" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log

if [ $ReturnCode -ne 0 ]; then
	echo "  **** Unable To Complete Summarization Due To Following Errors ****" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
	cat $RANGERHOME/tmp/DUAlarmDetectionLeadTimeSummary.log >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log 
	echo -e "     ********************************************************************\n\n\n\n" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
	rm -f $RANGERHOME/tmp/DUAlarmDetectionLeadTimeSummary.log
	exit 1
fi

echo "    > Deleted  : `grep "deleted" $RANGERHOME/tmp/DUAlarmDetectionLeadTimeSummary.log`" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
echo "    > Inserted : `grep "created" $RANGERHOME/tmp/DUAlarmDetectionLeadTimeSummary.log`" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
echo -e "     ********************************************************************\n\n\n\n" >> $RANGERHOME/LOG/DUAlarmDetectionLeadTimeSummary.log
rm -f $RANGERHOME/tmp/DUAlarmDetectionLeadTimeSummary.log
exit 0
