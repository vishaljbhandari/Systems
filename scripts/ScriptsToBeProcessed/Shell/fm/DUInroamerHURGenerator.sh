#******************************************************************************
#  Copyright (c) Subex Limited 2008. All rights reserved. The copyright to the*
#  computer program(s) herein is the property of Subex Limited. The program(s)*
#  may be used and/or copied with the written permission from Subex Systems or*
#  in accordance with the terms and conditions stipulated in the              *
#  agreement/contract under which the program(s) have been supplied.          *
#******************************************************************************/

#!/bin/bash

StartDate=""
EndDate=""
TestRun=0

PrintUsage ()
{
	echo "--------------------------------------------------------------------------------"
	echo "Usage DUInroamerHURGenerator.sh <DD/MM/YYYY [DD/MM/YYYY]>"
	echo "--------------------------------------------------------------------------------"
	echo "    Eg : ./DUInroamerHURGenerator.sh  01/01/2007 07/01/2007"
	echo "         ./DUInroamerHURGenerator.sh  01/01/2007"
	echo "--------------------------------------------------------------------------------"
}

Initialize ()
{
	sqlplus -s /nolog << EOF > /dev/null 2>&1
		CONNECT_TO_SQL
		whenever sqlerror exit 5 ;
		whenever oserror exit 6 ;
		create or replace directory doc_path as '$RANGERHOME/RangerData/HUROutput/' ;
EOF
	return 0
}

GenerateHURInStandardFormat ()
{
	sqlplus -s /nolog << EOF >> $RANGERHOME/LOG/DUInroamerHURGenerator.log
		CONNECT_TO_SQL
		whenever sqlerror exit 5 ;
		whenever oserror exit 6 ;
		set serveroutput on ;
		set feedback off ;
		declare
			ReturnValue			number(1)		:= 0 ;

			StartDate			date			:= null ;
			EndDate				date			:= null ;
			ReportSequance		varchar2(10)	:= null ;
			OutputFileName		varchar2(1024)	:= null ;
			LineCount			number			:= 0 ;
			RowsCount			number			:= 0 ;

			Temp 				number(10)		:= 0 ;
			F_HHH 				number(10)		:= 0 ;
			F_MM  				number(10)		:= 0 ;
			F_SS  				number(10)		:= 0 ; 

			OutFileHandler		utl_file.file_type ;

			--======================================================================================================================================
			-- *** Collect all roamer partner where HUR is enabled (1) *** --
			--======================================================================================================================================
			cursor roamer_partner_list is select * from roamer_partner where hur_enabled = 1 ;

			--======================================================================================================================================
			-- *** Parse through all inroamers from a network who breached threshold *** --
			--======================================================================================================================================
			cursor set_b_data_list is select * from participating_inroamers ;

			cursor set_c_imsi_list is select distinct(imsi_number) imsi_number from participating_inroamers ;
			cursor set_c_data_list (st_date date, en_date date, imsi varchar2) is 
				select min(time_stamp) mindate, max(time_stamp) maxdate, destination, count(*) nc, sum(duration_volume) dc, sum(value) sdr from du_hur_summary 
				where stream_type = 'C' and imsi_number = imsi and trunc(time_stamp) between trunc(st_date) and trunc(en_date) 
				group by destination ;

			--======================================================================================================================================
			-- *** Function to validate date time (string), if not valid will put null into ValidDate *** --
			--======================================================================================================================================
			function ValidateDate (TimeStamp in varchar2, ValidDate out date) return number is
			begin
				select to_date(nvl(TimeStamp, '---'), 'dd/mm/yyyyhh24:mi:ss') into ValidDate from dual ;
				return 0 ;
			exception
				when others then
					ValidDate := null ;
					return 1 ;
			end ;

			--======================================================================================================================================
			-- *** Function to check, is sequance reset is needed, if needed return 1  *** --
			--======================================================================================================================================
			function ResetSequance (CurrentSequance in varchar2) return number is
			begin
				if substr(CurrentSequance, 5) = to_char(sysdate, 'yyyy') then
					return 0 ;
				end if ;
				return 1 ;
			end ;

			--======================================================================================================================================
			-- *** Function to convert duration in seconds to HHHMMSS format.  *** --
			--======================================================================================================================================
			function DC_HHHMMSS (Duration number) return varchar2 is
			begin
				Temp 	:= Duration ;
				F_HHH 	:= floor(Temp / 3600) ;
				Temp 	:= Temp - (F_HHH * 3600) ;
				F_MM  	:= floor(Temp / 60) ;
				Temp 	:= Temp - (F_MM * 60) ;
				F_SS  	:= Temp ; 
				return (lpad (F_HHH, 3, 0) || lpad (F_MM, 2, 0) || lpad (F_SS, 2, 0)) ;
			end ;

			--======================================================================================================================================
			-- *** Function to print line to a file, also increments a counts to keep track of number of lines printed. *** --
			--======================================================================================================================================
			procedure PrintLine (Line in varchar2) is
			begin
				utl_file.put_line (OutFileHandler, Line) ;
				LineCount := LineCount + 1 ;
			end ;

		begin
			--======================================================================================================================================
			-- *** Validate Input Date parameters... *** --
			--======================================================================================================================================
			ReturnValue := ValidateDate ('$StartDate', StartDate) ;
			if ReturnValue <> 0 then
				dbms_output.put_line ('Invalid Start Date [$StartDate (dd/mm/yyyyhh24:mi:ss)]') ;	
				return ;
			end if;

			ReturnValue := ValidateDate ('$EndDate', EndDate) ;
			if ReturnValue <> 0 then
				dbms_output.put_line ('Invalid End Date [$EndDate (dd/mm/yyyyhh24:mi:ss)]') ;	
				return ;
			end if;

			--======================================================================================================================================
			-- *** Prepare LOG *** --
			--======================================================================================================================================
			dbms_output.put_line ('     ****************    DU Inroamer HUR Generation     *****************') ;
			dbms_output.put_line ('  Start Date : ' || to_char(sysdate, 'dd/mm/yyyy') || '            Start Time : ' || to_char(sysdate, 'hh24:mi:ss')) ;
			dbms_output.put_line ('     --------------------------------------------------------------------') ;
			dbms_output.put_line ('  Generation Of HUR For ... ') ;
			dbms_output.put_line ('     HUR Lower Bound : ' || to_char(StartDate, 'yyyy/mm/dd hh24:mi:ss')) ;
			dbms_output.put_line ('     HUR Upper Bound : ' || to_char(EndDate, 'yyyy/mm/dd hh24:mi:ss')) ;
			dbms_output.put_line ('     --------------------------------------------------------------------') ;

			for roamer_partner_rec in roamer_partner_list loop
				LineCount := 0 ;
				ReportSequance := lpad(nvl(roamer_partner_rec.inbound_sequence_number, '0000' || to_char(sysdate, 'yyyy')) + 10000, 8, 0) ;
				dbms_output.put_line (' > Started Generating HUR For : ' || roamer_partner_rec.tap_code) ;
				dbms_output.put_line (' > Sequance Number : ' || ReportSequance) ;

				if ResetSequance (ReportSequance) = 1 then
					ReportSequance := lpad ('0001' || to_char(sysdate, 'yyyy'), 8, 0) ;
				end if ;

				--======================================================================================================================================
				-- *** Create the output file, which is a combination of                  *** --
				-- *** (<FROM><TO><SEQUANCE>.csv)~<EMAIL ID's>~<IS NOTIFICATION DISABLED> *** --
				-- *** (HUR_ALLAREDUAIRTELINDIA00012008.csv~hur@airtel.com~0)	
				--======================================================================================================================================
				OutFileHandler := utl_file.fopen ('DOC_PATH', 'HUR_ALLAREDU' || 
													roamer_partner_rec.tap_code || 
													ReportSequance || '.csv' || '~' || 
													replace(replace(roamer_partner_rec.email_id, ' ', ''), ';', ',') || '~' || 
													nvl(roamer_partner_rec.block_empty_reports, 0), 'w') ; 

				PrintLine ('R') ;
				PrintLine ('R,SENDER,RECIPIENT,SEQUENCE NO,THRESHOLD,DATE AND TIME OF ANALYSIS,DATE AND TIME OF REPORT CREATION') ;
				PrintLine ('R') ;
				PrintLine ('H,AREDU,' || roamer_partner_rec.tap_code || ',' || ReportSequance || ',' || roamer_partner_rec.inbound_threshold || ',' || to_char(sysdate, 'yyyymmddhh24miss') || ',' || to_char(sysdate, 'yyyymmddhh24miss')) ;
				PrintLine ('R') ;
				PrintLine ('R') ;

				delete participating_inroamers ;

				--======================================================================================================================================
				-- *** Collect all inroamer records into temp table *** --
				--======================================================================================================================================
				insert into participating_inroamers
					select roamer_partner, imsi_number, sum(value) total_value, sum(duration_volume) metrix, min(time_stamp) start_time, 
						max(time_stamp) end_time, count(*) nc, sum(gprs_duration) gprs_duration, stream_type
					from du_hur_summary
					where roamer_partner = roamer_partner_rec.tap_code and time_stamp between StartDate and EndDate
					group by roamer_partner, imsi_number, stream_type ;

				--======================================================================================================================================
				-- *** Delete all inroamer records who did not breach threshold *** --
				--======================================================================================================================================
				delete from participating_inroamers 
					where imsi_number in 
						(select imsi_number 
						 from participating_inroamers, roamer_partner 
						 where roamer_partner = tap_code 
						 group by imsi_number 
						 having sum(total_value) <= roamer_partner_rec.inbound_threshold) ;

				--======================================================================================================================================
				-- *** Get the count of subscribers who breached threshold *** --
				--======================================================================================================================================
				select count(*) into RowsCount from participating_inroamers ;
				
				dbms_output.put_line (' > Breach Count : ' || RowsCount) ;

				if RowsCount > 0 then
					--======================================================================================================================================
					-- *** If count > 0, means there are subscribers who breached threshold. *** --
					--======================================================================================================================================
					dbms_output.put_line (' > Type Of Report Generated : HUR') ;
					PrintLine ('R,IMSI,DATE FIRST EVENT,TIME FIRST EVENT,DATE LAST EVENT,TIME LAST EVENT,DC(HHHMMSS),NC,VOLUME,SDR') ;
					PrintLine ('R') ;
					PrintLine ('R') ;
					--======================================================================================================================================
					-- *** Print Section - B using summarized data. (Stream Type P indicates Packet Switched and C indicates Circuit Switched *** --
					--======================================================================================================================================
					for set_b_record in set_b_data_list loop
						if set_b_record.stream_type = 'P' then
							PrintLine ('P,' || set_b_record.imsi_number 
											|| ',' || to_char(set_b_record.start_time, 'yyyymmdd') 
											|| ',' || to_char(set_b_record.start_time, 'hh24:mi:ss')
											|| ',' || to_char(set_b_record.end_time, 'yyyymmdd') 
											|| ',' || to_char(set_b_record.end_time, 'hh24:mi:ss')
											|| ',' || DC_HHHMMSS (set_b_record.gprs_duration)
											|| ',' || set_b_record.call_count
											|| ',' || set_b_record.metrix
											|| ',' || set_b_record.total_value) ;
						else
							PrintLine ('C,' || set_b_record.imsi_number 
											|| ',' || to_char(set_b_record.start_time, 'yyyymmdd')
											|| ',' || to_char(set_b_record.start_time, 'hh24:mi:ss')
											|| ',' || to_char(set_b_record.end_time, 'yyyymmdd')
											|| ',' || to_char(set_b_record.end_time, 'hh24:mi:ss')
											|| ',' || DC_HHHMMSS (set_b_record.metrix)
											|| ',' || set_b_record.call_count
											|| ',' 
											|| ',' || set_b_record.total_value) ;
						end if ;
					end loop ;
					PrintLine ('R') ;
					PrintLine ('R') ;
					PrintLine ('R') ;
					PrintLine ('R,IMSI,DATE FIRST EVENT,DATE LAST EVENT,DESTINATION OF EVENTS,NC,DC(HHHMMSS),SDR') ;
					
					--======================================================================================================================================
					-- *** Print Section - C using Distinct of IMSI's from the collected summary of section B *** --
					-- *** This section contains only circuit switched records as destination is mandatory    *** --
					-- *** field and is not present for GPRS records.                                         *** --
					--======================================================================================================================================
					for imsi in set_c_imsi_list loop
						for rec_list in set_c_data_list (StartDate, EndDate, imsi.imsi_number) loop
							PrintLine ('A,' || imsi.imsi_number
											|| ',' || to_char(rec_list.mindate, 'yyyymmdd')
											|| ',' || to_char(rec_list.maxdate, 'yyyymmdd')
											|| ',' || substr(rec_list.destination, 1, 6)
											|| ',' || rec_list.nc
											|| ',' || DC_HHHMMSS (rec_list.dc)
											|| ',' || rec_list.sdr) ;
						end loop ;
					end loop ;

					PrintLine ('R') ;
					PrintLine ('R,END OF REPORT') ;
					PrintLine ('T,' || (LineCount + 1)) ;
				else
					--======================================================================================================================================
					-- *** If count <= 0, means there are no subscribers who breached threshold, hense a notification report *** --
					--======================================================================================================================================
					dbms_output.put_line (' > Type Of Report Generated : Notification') ;
					PrintLine ('R,BEGINNING OF THE OBSERVATION PERIOD,END OF THE OBSERVATION PERIOD') ;
					PrintLine ('N,' || to_char (StartDate, 'yyyymmdd') || ',' || to_char (EndDate, 'yyyymmdd')) ;
					PrintLine ('R') ;
					PrintLine ('R') ;
					PrintLine ('R') ;
					PrintLine ('R,END OF REPORT') ;
					PrintLine ('R') ;
					PrintLine ('R,NONE OF THE IMSIS EXCEEDED THE AGREED THRESHOLD ON THIS PMN') ;
					PrintLine ('R') ;
					PrintLine ('T,' || (LineCount + 1)) ;
				end if ;
				utl_file.fclose (OutFileHandler) ;

				--======================================================================================================================================
				-- *** Update all required fields, including sequance number *** --
				--======================================================================================================================================
				update roamer_partner set inbound_last_processed_time = sysdate where tap_code = roamer_partner_rec.tap_code ;
				if roamer_partner_rec.block_empty_reports = 0 or RowsCount > 0 then
					update roamer_partner set inbound_sequence_number = ReportSequance where tap_code = roamer_partner_rec.tap_code ;
				end if ;
				dbms_output.put_line (' * - * - * - * - * ') ;
			end loop ;
			dbms_output.put_line ('     --------------------------------------------------------------------') ;
			dbms_output.put_line ('  End Date   : ' || to_char(sysdate, 'dd/mm/yyyy') || '            End Time   : ' || to_char(sysdate, 'hh24:mi:ss')) ;
			dbms_output.put_line ('     ********************************************************************') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
			dbms_output.put_line ('~') ;
		end ;
/
EOF
	return 0
}

SendMail ()
{
	for File in `ls $RANGERHOME/RangerData/HUROutput`
	do
		if [ ! -d $RANGERHOME/RangerData/HUROutput/$File ]; then
			FileName=`echo $File | cut -d'~' -f1`
			E_MailID=`echo $File | cut -d'~' -f2`
			BlockEmptyReports=`echo $File | cut -d'~' -f3`
			mv $RANGERHOME/RangerData/HUROutput/$File $RANGERHOME/RangerData/HUROutput/$FileName
			if [ $TestRun -ne 1 ]; then
				if [ $BlockEmptyReports -eq 1 ]; then
					grep "NONE OF THE IMSIS EXCEEDED THE AGREED THRESHOLD ON THIS PMN" $RANGERHOME/RangerData/HUROutput/$FileName
					if [ $? -ne 0 ]; then
					uuencode $RANGERHOME/RangerData/HUROutput/$FileName $RANGERHOME/RangerData/HUROutput/$FileName | mailx -s "HUR From DU - $FileName" -r "hur@du.ae" "$E_MailID"
#					uuencode $RANGERHOME/RangerData/HUROutput/$FileName $RANGERHOME/RangerData/HUROutput/$FileName | mailx -s "HUR From DU - $FileName" -r "hur@du.ae" "hari.kumar@subexworld.com"
					fi
				else
					uuencode $RANGERHOME/RangerData/HUROutput/$FileName $RANGERHOME/RangerData/HUROutput/$FileName | mailx -s "HUR From DU - $FileName" -r "hur@du.ae" "$E_MailID"
#					uuencode $RANGERHOME/RangerData/HUROutput/$FileName $RANGERHOME/RangerData/HUROutput/$FileName | mailx -s "HUR From DU - $FileName" -r "hur@du.ae" "hari.kumar@subexworld.com"
				fi	
			fi
			cp $RANGERHOME/RangerData/HUROutput/$FileName $RANGERHOME/RangerData/HUROutput/Backup/
			rm -f $RANGERHOME/RangerData/HUROutput/$FileName
		fi
	done
}

Main ()
{
	Initialize
	if [ $? -ne 0 ]; then
		echo "DUInroamerHURGenerator : Initialize Failed ... "
		exit 1
	fi

	GenerateHURInStandardFormat
	if [ $? -ne 0 ]; then
		echo "DUInroamerHURGenerator : GenerateHURInStandardFormat Failed ... "
		exit 2
	fi

	SendMail
	if [ $? -ne 0 ]; then
		echo "DUInroamerHURGenerator : Sending Mails Failed ... "
		exit 3
	fi
}

if ! [ $# -ge 1 -a $# -le 2 ]
then
	PrintUsage && exit 1
fi

StartDate=$1

if [ $# -eq 2 ]; then
	EndDate=$2
else
	EndDate=$1
fi

StartDate=$StartDate"00:00:00"
EndDate=$EndDate"23:59:59"

Main
