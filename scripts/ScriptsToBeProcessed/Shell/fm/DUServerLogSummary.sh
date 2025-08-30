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

SERVER_STARTED=728
SERVER_SHOUTDOWN=729

. rangerenv.sh

if [ -z "$NetworkID" ]; then
	NetworkID="999"
fi

on_exit ()
{
	rm -f $RANGERHOME/RangerData/Scheduler/DUServerLogSummary$NetworkID
}
trap on_exit EXIT

DUServerStatusSummary()
{
	sqlplus -s /nolog << EOF > $RANGERHOME/tmp/DUServerLogSummary.log
	CONNECT_TO_SQL
		whenever sqlerror exit 5 ;
		whenever oserror  exit 6 ;
		set serveroutput on ;

		declare
			max_log_id		number := 0 ;
		begin
			select nvl((select max(fms_log_table_id) from du_server_status_summary), 0) into max_log_id from dual ;	
			insert into du_server_status_summary 
				(date_time, fms_log_id, fms_log_table_id)
					(select 
						logged_date, event_code, id
				 	 from
				 		audit_trails	
				 	 where
				 		event_code in ($SERVER_STARTED, $SERVER_SHOUTDOWN) 
						and id > max_log_id)
			;
			dbms_output.put_line (sql%rowcount || ' rows created') ;
		end ;
/
EOF
	return $?
}

echo "     ************** DU Server StartUp And ShutDown Summary **************" >> $RANGERHOME/LOG/DUServerLogSummary.log
echo -n "  Start Date : `date +%Y/%m/%d`          " >> $RANGERHOME/LOG/DUServerLogSummary.log
echo "  Start Time : `date +%T`" >> $RANGERHOME/LOG/DUServerLogSummary.log

DUServerStatusSummary

ReturnCode=$?

echo -n "  End Date   : `date +%Y/%m/%d`          " >> $RANGERHOME/LOG/DUServerLogSummary.log
echo "  End Time   : `date +%T`" >> $RANGERHOME/LOG/DUServerLogSummary.log
echo "     --------------------------------------------------------------------" >> $RANGERHOME/LOG/DUServerLogSummary.log
echo "  Network ID                 : $NetworkID" >> $RANGERHOME/LOG/DUServerLogSummary.log
echo "     --------------------------------------------------------------------" >> $RANGERHOME/LOG/DUServerLogSummary.log

if [ $ReturnCode -ne 0 ]; then
	echo "  **** Unable To Complete Summarization Due To Following Errors ****" >> $RANGERHOME/LOG/DUServerLogSummary.log
	cat $RANGERHOME/tmp/DUServerLogSummary.log >> $RANGERHOME/LOG/DUServerLogSummary.log 
	echo -e "     ********************************************************************\n\n\n\n" >> $RANGERHOME/LOG/DUServerLogSummary.log
	rm -f $RANGERHOME/tmp/DUServerLogSummary.log
	exit 1
fi

echo "    > Inserted : `grep "created" $RANGERHOME/tmp/DUServerLogSummary.log`" >> $RANGERHOME/LOG/DUServerLogSummary.log
echo -e "     ********************************************************************\n\n\n\n" >> $RANGERHOME/LOG/DUServerLogSummary.log
rm -f $RANGERHOME/tmp/DUServerLogSummary.log
exit 0
