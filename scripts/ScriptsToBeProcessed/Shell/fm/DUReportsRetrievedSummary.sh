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

REPORT_RETRIVED_EVENT_CODE=727
. rangerenv.sh

if [ -z "$NetworkID" ]; then
	NetworkID="999"
fi

on_exit ()
{
	rm -f $RANGERHOME/RangerData/Scheduler/DUReportsRetrievedSummaryPID$NetworkID
}
trap on_exit EXIT

DUReportsRetrivedSummary ()
{
	sqlplus -s /nolog << EOF > $RANGERHOME/tmp/DUReportsRetrievedSummary.log
	CONNECT_TO_SQL
		whenever sqlerror exit 5 ;
		whenever oserror  exit 6 ;
		set serveroutput on ;
		declare
			max_log_id		number := 0 ;
		begin
			select nvl((select max(audit_trails_table_id) from du_reports_retrieved_summary), 0) into max_log_id from dual ;	
			insert into du_reports_retrieved_summary
				(date_time,user_id,audit_trails_table_id)
				(select 
			 		logged_date, user_id, id
				 from
					audit_trails
				 where 
				 	event_code = $REPORT_RETRIVED_EVENT_CODE and id > max_log_id) ;
			dbms_output.put_line (sql%rowcount || ' rows created') ;
		end ;
/
EOF
	return $?
}

echo "     ************** DU Reports Retrived By Analyst Summary **************" >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log
echo -n "  Start Date : `date +%Y/%m/%d`          " >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log
echo "  Start Time : `date +%T`" >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log

DUReportsRetrivedSummary

ReturnCode=$?

echo -n "  End Date   : `date +%Y/%m/%d`          " >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log
echo "  End Time   : `date +%T`" >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log
echo "     --------------------------------------------------------------------" >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log
echo "  Network ID                 : $NetworkID" >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log
echo "     --------------------------------------------------------------------" >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log

if [ $ReturnCode -ne 0 ]; then
	echo "  **** Unable To Complete Summarization Due To Following Errors ****" >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log
	cat $RANGERHOME/tmp/DUReportsRetrievedSummary.log >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log 
	echo -e "     ********************************************************************\n\n\n\n" >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log
	rm -f $RANGERHOME/tmp/DUReportsRetrievedSummary.log
	exit 1
fi

echo "    > Inserted : `grep "created" $RANGERHOME/tmp/DUReportsRetrievedSummary.log`" >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log
echo -e "     ********************************************************************\n\n\n\n" >> $RANGERHOME/LOG/DUReportsRetrievedSummary.log
rm -f $RANGERHOME/tmp/DUReportsRetrievedSummary.log
exit 0
