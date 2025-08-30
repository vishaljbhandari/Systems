#******************************************************************************
#  Copyright (c) Subex Limited 2008. All rights reserved. The copyright to the*
#  computer program(s) herein is the property of Subex Limited. The program(s)*
#  may be used and/or copied with the written permission from Subex Systems or*
#  in accordance with the terms and conditions stipulated in the              *
#  agreement/contract under which the program(s) have been supplied.          *
#******************************************************************************/

#!/bin/bash

NetworkID=$1
DateToSummarize=$2

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
	rm -f $RANGERHOME/RangerData/Scheduler/DUInroamerHURSummary
}
trap on_exit EXIT

SummarizeXDRData ()
{
	sqlplus -s /nolog << EOF >> $RANGERHOME/LOG/DUInroamerHURSummary.log
	CONNECT_TO_SQL
		set serveroutput on ;
		set feedback off ;

		declare
			-- LOG Related Variables.
			rows_deleted			number(15) := 0 ;
			gsm_rows_inserted		number(15) := 0 ;
			gprs_rows_inserted		number(15) := 0 ;
			record_commit_count		number(15) := 0 ;

			sdr_value			number(16,6) := null ;
			is_match			number(1) := null ;

			cursor phone_number_list is select unique(destination) from du_hur_summary_temp ;
			cursor imsi_number_list is select unique(imsi_number) from du_hur_summary_temp ;

			type country_codes_type is table of country_codes%rowtype index by pls_integer;
			type roamer_partners is table of roamer_partner%rowtype index by pls_integer;
			allcodes country_codes_type ;
			allroamerpartner roamer_partners ;

		begin
			dbms_output.put_line ('     ****************      DU Inroamer HUR Summary      *****************') ;
			dbms_output.put_line ('  Start Date : ' || to_char(sysdate, 'dd/mm/yyyy') || '            Start Time : ' || to_char(sysdate, 'hh24:mi:ss')) ;

			select * bulk collect into allcodes from country_codes order by length(code) desc;
			select * bulk collect into allroamerpartner from roamer_partner order by length(starting_imsi) desc;

			--======================================================================================================================================
			-- *** Delete summary records which belongs to summarizing date or which are older then the retention period *** --
			--======================================================================================================================================
			delete from du_hur_summary 
				where 
					trunc(time_stamp) < trunc(sysdate - (select nvl ((select value from configurations where config_key = 'DU.CDR.SUMMARY.CLEANUP.DAYS'),15) from dual)) or
					trunc(time_stamp) = to_date('$DateToSummarize', 'dd/mm/yyyy') ;
			rows_deleted := sql%rowcount ;
			--======================================================================================================================================

			delete du_hur_summary_temp ;
			commit;
			--======================================================================================================================================
			-- Select all inroamer CDR's belonging to summarization date, and insert them into temp table.
			--======================================================================================================================================
			insert into /*+ append */ du_hur_summary_temp
				select 
					imsi_number, time_stamp, decode(record_type, 2, '*****', 1, called_number, 3, forwarded_number) destination, ' ' roamer_partner, duration, value, 0 gprs_duration, 'C' stream_type
				from 
					cdr
				where 
					day_of_year = to_char(to_date('$DateToSummarize', 'dd/mm/yyyy'), 'ddd') and 
					cdr_type = 6 and
					length(imsi_number) > 6 ;
			gsm_rows_inserted := sql%rowcount ;
			commit;
			--======================================================================================================================================
			-- Select all inroamer GPRS CDR's belonging to summarization date, and insert them into temp table.
			--======================================================================================================================================
			insert into /*+ append */ du_hur_summary_temp
				select 
					imsi_number, time_stamp, null destination, ' ' roamer_partner, round((upload_data_volume + download_data_volume) * 1048576), value, duration, 'P' stream_type
				from 
					gprs_cdr
				where 
					day_of_year = to_char(to_date('$DateToSummarize', 'dd/mm/yyyy'), 'ddd') and 
					cdr_type = 6 and
					length(imsi_number) > 6 ;
			gprs_rows_inserted := sql%rowcount ;
			commit;
			--======================================================================================================================================
			-- Populate destination country code using destination field, this implements a LPM.
			--======================================================================================================================================
			record_commit_count := 0;
			for phone_number_field in phone_number_list loop
				is_match := 0 ;
				for i in allcodes.first..allcodes.last loop
					if SUBSTR(phone_number_field.destination,1,length(allcodes(i).code)) = allcodes(i).code then
							is_match := 1 ;
					end if ;

					if is_match = 1 then
						update du_hur_summary_temp set destination = allcodes(i).code where destination = phone_number_field.destination ;
						record_commit_count := record_commit_count + 1 ;
						if record_commit_count = 500 then
							commit ;
							record_commit_count := 0 ;
						end if ; 
						exit ;
					end if ;
				end loop ;
			end loop ;
				update du_hur_summary_temp set destination = 'PR' where destination = (select phone_number from prs_number where phone_number = destination) ;

			commit;
			--======================================================================================================================================
			-- Populate roamer partner TAP_CODE code using imsi field, this implements a LPM.
			--======================================================================================================================================
			record_commit_count := 0;
			for imsi_number_field in imsi_number_list loop
				is_match := 0 ;
				for i in 1..allroamerpartner.count loop
					if SUBSTR(imsi_number_field.imsi_number,1,length(allroamerpartner(i).starting_imsi)) = allroamerpartner(i).starting_imsi then
							is_match := 1 ;
					end if ;

					if is_match = 1 then
						update du_hur_summary_temp set roamer_partner = allroamerpartner(i).tap_code where imsi_number = imsi_number_field.imsi_number ;
						record_commit_count := record_commit_count + 1 ;
						if record_commit_count = 500 then
							commit ;
							record_commit_count := 0 ;
						end if ; 
						exit ;
					end if ;
				end loop ;
			end loop ;
				update du_hur_summary_temp set destination = 'PR' where destination = (select phone_number from prs_number where phone_number = destination) ;
			commit;

			--======================================================================================================================================
			-- Update Value field by converting it to SDR (get_sdr_value routine is a custom procedure).
			--======================================================================================================================================
			update du_hur_summary_temp set value = get_sdr_value (value, time_stamp) ;

			--======================================================================================================================================
			-- Merge all processed records present in temp table into summary table.
			--======================================================================================================================================
			insert into du_hur_summary select * from  du_hur_summary_temp ;
			commit;

			dbms_output.put_line ('  End Date   : ' || to_char(sysdate, 'dd/mm/yyyy') || '            End Time   : ' || to_char(sysdate, 'hh24:mi:ss')) ;
			dbms_output.put_line ('     --------------------------------------------------------------------') ;
			dbms_output.put_line ('  Summarized Date : $DateToSummarize') ; 
			dbms_output.put_line ('     --------------------------------------------------------------------') ;
			dbms_output.put_line ('  Total GSM Inroamer Records Inserted   : ' || gsm_rows_inserted) ;
			dbms_output.put_line ('  Total GPRS Inroamer Records Inserted  : ' || gprs_rows_inserted) ;
			dbms_output.put_line ('  Total Inroamer Records Inserted       : ' || (gsm_rows_inserted + gprs_rows_inserted)) ;
			dbms_output.put_line ('     --------------------------------------------------------------------') ;
			dbms_output.put_line ('  Total Duplicate Records (XDR) Deleted : ' || rows_deleted) ;
			dbms_output.put_line ('     ********************************************************************') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
		end ;
/
EOF
}

main ()
{
	SummarizeXDRData
	if [ $? -ne 0 ]; then
		echo "Summarizing Voice/SMS/GPRS/MMS Data Failed !"
	else
		echo "Summarizing Voice/SMS/GPRS/MMS Data Success !"
	fi
}

main
