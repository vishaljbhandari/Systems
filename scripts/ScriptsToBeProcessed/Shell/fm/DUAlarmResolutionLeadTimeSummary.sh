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
	rm -f $RANGERHOME/RangerData/Scheduler/DUAlarmResolutionLeadTime$NetworkID
}
trap on_exit EXIT

UpdateAlarmResolutionSummary()
{
	sqlplus -s /nolog << EOF > $RANGERHOME/tmp/DUAlarmResolutionLeadTimeSummary.log
		CONNECT_TO_SQL
		whenever sqlerror exit 5 ;
   		whenever oserror  exit 6 ;
		
		delete from du_alarm_resolution_summary 
			where 
				trunc(alarm_created_date) = to_date('$DateToSummarize', 'dd/mm/yyyy')
				or trunc(alarm_created_date) < to_date('$DateToSummarize', 'dd/mm/yyyy') - 
					(select value from configuration where id like 'Alarm Cleanup-Interval in Days%') ;

		insert into du_alarm_resolution_summary
			(user_id, alarm_created_date, resolution_time)
			(select user_id, created_date, modified_date - created_date 
				from 
					alarms
				where
					trunc(modified_date) = to_date('$DateToSummarize', 'dd/mm/yyyy')
					and status in (2, 4)) ;
		quit ;
EOF
}

echo "     *************** DU Alarm Resolution Lead Time Summary **************" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
echo -n "  Start Date : `date +%Y/%m/%d`          " >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
echo "  Start Time : `date +%T`" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log

UpdateAlarmResolutionSummary

ReturnCode=$?

echo -n "  End Date   : `date +%Y/%m/%d`          " >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
echo "  End Time   : `date +%T`" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
echo "     --------------------------------------------------------------------" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
echo "  Summarization Run For Date : $DateToSummarize" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
echo "  Network ID                 : $NetworkID" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
echo "     --------------------------------------------------------------------" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log

if [ $ReturnCode -ne 0 ]; then
	echo "  **** Unable To Complete Summarization Due To Following Errors ****" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
	cat $RANGERHOME/tmp/DUAlarmResolutionLeadTimeSummary.log >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log 
	echo -e "     ********************************************************************\n\n\n\n" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
	rm -f $RANGERHOME/tmp/DUAlarmResolutionLeadTimeSummary.log
	exit 1
fi

echo "    > Deleted  : `grep "deleted" $RANGERHOME/tmp/DUAlarmResolutionLeadTimeSummary.log`" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
echo "    > Inserted : `grep "created" $RANGERHOME/tmp/DUAlarmResolutionLeadTimeSummary.log`" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
echo -e "     ********************************************************************\n\n\n\n" >> $RANGERHOME/LOG/DUAlarmResolutionLeadTimeSummary.log
rm -f $RANGERHOME/tmp/DUAlarmResolutionLeadTimeSummary.log
exit 0
