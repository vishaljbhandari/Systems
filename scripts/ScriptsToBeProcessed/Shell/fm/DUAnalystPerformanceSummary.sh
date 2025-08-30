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

SUBSCRIBER_REF_TYPE=1

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

echo $DateToSummarize

on_exit ()
{
	rm -f $RANGERHOME/RangerData/Scheduler/DUanalystPerformanceSummary"$NetworkID"
}
trap on_exit EXIT

function AnalystPerformanceSummary
{
	sqlplus -s /nolog << EOF >> $RANGERHOME/LOG/DUAnalystPerformance.log
	CONNECT_TO_SQL
		set serveroutput on ;
		set feedback off ;
	declare

		alarm_status		varchar2(64) := '' ;
		phonenumber			varchar2(40) := '' ;
		alarm_id			number := 0 ;
		rows_inserted		number := 0 ;
		rows_ignored		number := 0 ;
		rows_deleted		number := 0 ;

		type user_id_array is table of alarm_activity_logs.user_id%type index by binary_integer ;
		type time_stamp_array is table of alarm_activity_logs.log_date%type index by binary_integer ;
		type action_array is table of alarm_activity_logs.activity_description%type index by binary_integer ;
		type alarm_id_array is table of alarm_activity_logs.alarm_id%type index by binary_integer ;

		type subscription_type_array is table of subscriber.subscription_type%type index by binary_integer ;
		type product_type_array is table of subscriber.product_type%type index by binary_integer ;
		type groups_array is table of subscriber.groups%type index by binary_integer ;

		user_ids user_id_array ;
		time_stamps time_stamp_array ;
		actions action_array ;
		alarm_ids alarm_id_array ;

		subscriptiontypes_a subscription_type_array ;
		producttypes_a product_type_array ;
		groups_a groups_array ;


		cursor alarm_activity_list(date_to_summarize varchar2) is
			select user_id,log_date,activity_description,alarm_id
			from
				alarm_activity_logs	
			where
				trunc(log_date) = to_date(date_to_summarize, 'dd/mm/yyyy')
				and user_id not in ('ranger', 'default') ;

		cursor subscriber_loop(phone_number_in varchar2) is
			select groups, product_type,subscription_type
			from
				subscriber
			where
				phone_number = phone_number_in
				and status in (0,1,2) 
			order by
				subscriber_doa desc ;

		begin

			dbms_output.put_line ('     ****************   DU Analyst Performance Summary   ****************') ;
			dbms_output.put_line ('  Start Date : ' || to_char(sysdate, 'dd/mm/yyyy') || '            Start Time : ' || to_char(sysdate, 'hh24:mi:ss')) ;

			delete from du_analyst_performance_summary where trunc(log_time) = trunc(to_date('$DateToSummarize', 'dd/mm/yyyy')) ;
			rows_deleted := sql%rowcount ;
			open alarm_activity_list ('$DateToSummarize') ;
			fetch alarm_activity_list bulk collect into user_ids,time_stamps,actions,alarm_ids ;

			for i in 1 .. user_ids.count
			loop
				alarm_status	:= substr(actions(i),instr(actions(i),' ',1,4)+1) ;
				select reference_value into phonenumber from alarms where id = alarm_ids(i) and reference_type = $SUBSCRIBER_REF_TYPE ;

				open subscriber_loop(phonenumber) ;
				fetch subscriber_loop bulk collect into groups_a,producttypes_a,subscriptiontypes_a ;

				if ( alarm_status not in ('Investigated to Investigated', 'Altered to Altered') ) then

					if ( groups_a.count > 0 ) then
						insert into 
							du_analyst_performance_summary(user_id,log_time,status_change,groups,product_type,subscription_type,alarm_id) 
						values
							(user_ids(i),time_stamps(i),alarm_status,nvl(groups_a(1),'Default'),producttypes_a(1),nvl(subscriptiontypes_a(1),'Default'),alarm_ids(i)) ;
					else
						insert into 
							du_analyst_performance_summary(user_id,log_time,status_change,groups,product_type,subscription_type,alarm_id) 
						values
							(user_ids(i),time_stamps(i),alarm_status,'Default',0,'Default',alarm_ids(i)) ;
					end if ;

					rows_inserted := rows_inserted + 1 ;

				end if ;

				close subscriber_loop ;

			end loop ;

			close alarm_activity_list ;

			dbms_output.put_line ('  End Date   : ' || to_char(sysdate, 'dd/mm/yyyy') || '            End Time   : ' || to_char(sysdate, 'hh24:mi:ss')) ;
			dbms_output.put_line ('     --------------------------------------------------------------------') ;
			dbms_output.put_line ('  Summarization Run For Date : $DateToSummarize') ; 
			dbms_output.put_line ('  Network ID  	               : $NetworkID') ;
			dbms_output.put_line ('     --------------------------------------------------------------------') ;
			dbms_output.put_line ('  Total Records Processed : ' || user_ids.count) ;
			dbms_output.put_line ('  Total Records Inserted  : ' || rows_inserted) ;
			dbms_output.put_line ('  Total Records Deleted   : ' || rows_deleted) ;
			dbms_output.put_line ('     ********************************************************************') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
		end ;
/
EOF
}

AnalystPerformanceSummary
