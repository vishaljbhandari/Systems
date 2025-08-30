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

USER_LOGGED_IN_EVENT_CODE=636
USER_LOGGED_OUT_EVENT_CODE=637

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
	rm -f $RANGERHOME/RangerData/Scheduler/DUFMSLogSummary$NetworkID
}
trap on_exit EXIT

UserActivitySummary ()
{
	sqlplus -s /nolog << EOF >> $RANGERHOME/LOG/DUFMSLogSummary.log
	CONNECT_TO_SQL
		set serveroutput on ;
		set feedback off ;
		declare
			inserted_count			number(4) := 0 ;
			ignored_count			number(4) := 0 ;
			rows_deleted			number(4) := 0 ;

			type user_id_array is table of audit_trails.entity_value%type index by binary_integer ;
			type event_code_array is table of audit_trails.event_code%type index by binary_integer ;
			type time_stamp_array is table of audit_trails.logged_date%type index by binary_integer ;
			
			user_ids user_id_array ;
			event_codes event_code_array ;
			time_stamps time_stamp_array ;

			cursor audit_activity_list(date_to_summarize varchar2) is 
				select entity_value, event_code, logged_date 
					from 
						audit_trails
					where 
						trunc(logged_date) = to_date(date_to_summarize, 'dd/mm/yyyy')
						and event_code in ($USER_LOGGED_IN_EVENT_CODE, $USER_LOGGED_OUT_EVENT_CODE)
					order by
						entity_value, logged_date, event_code;
		begin
			dbms_output.put_line ('     ***************   DU Analyst Login Details Summary   ***************') ;
			dbms_output.put_line ('  Start Date : ' || to_char(sysdate, 'dd/mm/yyyy') || '            Start Time : ' || to_char(sysdate, 'hh24:mi:ss')) ;

			delete 
				from 
					du_fms_log_summary 
				where 
					trunc(login_time) = to_date('$DateToSummarize', 'dd/mm/yyyy')
					or trunc(login_time) <= to_date('$DateToSummarize', 'dd/mm/yyyy') 
							- nvl((select value from configuration where id = 'FMSLog Cleanup-Interval in Days'), 30) ;

			rows_deleted := SQL%ROWCOUNT;

			open audit_activity_list ('$DateToSummarize') ;
			fetch audit_activity_list bulk collect into user_ids, event_codes, time_stamps ;

			for i in 1 .. user_ids.count
			loop
				if event_codes(i) = $USER_LOGGED_IN_EVENT_CODE then
					if i + 1 <= user_ids.count and event_codes(i + 1) = $USER_LOGGED_OUT_EVENT_CODE then
						insert into du_fms_log_summary values (user_ids(i), time_stamps (i), time_stamps (i + 1)) ;
						inserted_count := inserted_count + 1 ;
					end if ;
				end if ;
			end loop ;
			close audit_activity_list ;
			ignored_count := user_ids.count - (inserted_count * 2) ;

			dbms_output.put_line ('  End Date   : ' || to_char(sysdate, 'dd/mm/yyyy') || '            End Time   : ' || to_char(sysdate, 'hh24:mi:ss')) ;
			dbms_output.put_line ('     --------------------------------------------------------------------') ;
			dbms_output.put_line ('  Summarization Run For Date : $DateToSummarize') ; 
			dbms_output.put_line ('  Network ID                 : $NetworkID') ;
			dbms_output.put_line ('     --------------------------------------------------------------------') ;
			dbms_output.put_line ('Total Record Processed : ' || user_ids.count) ;
			dbms_output.put_line ('Total Accepted Pairs   : ' || inserted_count) ;
			dbms_output.put_line ('Total Ignored Records  : ' || ignored_count) ;
			dbms_output.put_line ('Total Records Deleted  : ' || rows_deleted) ;
			dbms_output.put_line ('     ********************************************************************') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
		end ;
/
EOF
}

UserActivitySummary
