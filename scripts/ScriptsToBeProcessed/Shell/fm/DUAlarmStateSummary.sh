#! /bin/bash

. ~/RangerRoot/sbin/rangerenv.sh

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
	rm -f $RANGERHOME/RangerData/Scheduler/DUAlarmStateSummary"$NetworkID"
}
trap on_exit EXIT

DUAlarmStateSummary()
{
	sqlplus -s /nolog << EOF >> $RANGERHOME/LOG/DUAlarmStateSummary.log
	CONNECT_TO_SQL

	set feedback off ;
	set serveroutput on ;

	declare
		rows_inserted		number := 0 ;
		rows_deleted		number := 0 ;
		altered				number := 0 ;
		available           number := 0 ;
		fraud				number := 0 ;
		investigation       number := 0 ;
		nonfraud			number := 0 ;
		pending             number := 0 ;
		new_alarm			number := 0 ;

		cursor AlarmState is
				select count(*) as count,status from alarms where trunc(modified_date) = trunc(to_date('$DateToSummarize','dd/mm/yyyy')) and status between 0 and 6 group by status ;

		begin

		dbms_output.put_line ('     ****************   DU Alarm State Summary   ****************') ;
		dbms_output.put_line ('  Start Date : ' || to_char(sysdate, 'dd/mm/yyyy') || '            Start Time : ' || to_char(sysdate, 'hh24:mi:ss')) ;

		delete from du_alarm_state_summary where trunc(summary_date) = trunc(to_date('$DateToSummarize', 'dd/mm/yyyy')) ;

		rows_deleted := sql%rowcount ;
		
			for records in AlarmState loop
			begin
				case records.status
					when 0 then
						altered			:= records.count ;
					when 1 then
						available		:= records.count ;
					when 2 then
						fraud 			:= records.count ;
					when 3 then
						investigation 	:= records.count ;
					when 4 then
						nonfraud 		:= records.count ;
					when 5 then
						pending 		:= records.count ;
					when 6 then
						new_alarm 		:= records.count ;
				end case ;
			end ;
			end loop ;

		insert into du_alarm_state_summary values (to_date('$DateToSummarize','dd/mm/yyyy'),altered,available,fraud,investigation,nonfraud,pending,new_alarm) ;

		rows_inserted := rows_inserted + 1 ;

		dbms_output.put_line ('  End Date   : ' || to_char(sysdate, 'dd/mm/yyyy') || '            End Time   : ' || to_char(sysdate, 'hh24:mi:ss')) ;
		dbms_output.put_line ('     --------------------------------------------------------------------') ;
		dbms_output.put_line ('  Summarization Run For Date : $DateToSummarize') ; 
		dbms_output.put_line ('  Network ID  	               : $NetworkID') ;
		dbms_output.put_line ('     --------------------------------------------------------------------') ;
		dbms_output.put_line ('  Total Records Inserted  : ' || rows_inserted) ;
		dbms_output.put_line ('  Total Records Deleted   : ' || rows_deleted) ;
		dbms_output.put_line ('     ********************************************************************') ;
		dbms_output.put_line ('~') ;
		dbms_output.put_line ('~') ;
		dbms_output.put_line ('~') ;
		dbms_output.put_line ('~') ;

	end ;
/
	commit ;
quit ;
EOF
}

DUAlarmStateSummary 

