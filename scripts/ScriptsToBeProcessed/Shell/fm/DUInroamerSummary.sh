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
	rm -f $RANGERHOME/RangerData/Scheduler/DUInroamerCDRSummary$NetworkID
}
trap on_exit EXIT

sqlplus -s /nolog << EOF > $RANGERHOME/LOG/TempDUInroamerCDRSummaryLog.tmp 2>&1
CONNECT_TO_SQL
	whenever sqlerror exit 5 ;
	whenever oserror exit 6 ;
	set serveroutput on ;
	set feedback on ;
	
 	delete 
		from 
			du_inroamer_cdr_summary 
		where 
			time_stamp < sysdate - (select nvl ((select value from configurations where config_key = 'DU.INROAMER.CDR.SUMMARY.CLEANUP.DAYS'), 15) from dual) or
			trunc(time_stamp) = to_date('$DateToSummarize', 'dd/mm/yyyy');
	
	declare
		date_of_cdr date	:= null ;
		cdr_country_code	varchar2(40) := null ;
		cdr_network_code	varchar2(40) := null ;
		is_found			number := 0 ;

		cursor inroamer_cdr_list is select * from cdr where day_of_year = to_char(to_date('$DateToSummarize', 'dd/mm/yyyy'), 'ddd') 
			and cdr_type in (6, 8) and phone_number not like '+97155%' ;

		cursor inroamer_gprs_cdr_list is select * from gprs_cdr where day_of_year = to_char(to_date('$DateToSummarize', 'dd/mm/yyyy'), 'ddd') 
			and cdr_type in (6, 8) and phone_number not like '+97155%' ;

		type code_table is table of country_codes%rowtype index by pls_integer;
		type roamer_table is table of roamer_partner%rowtype index by pls_integer;

        allcodes code_table ;
		allroamerpartner roamer_table ;
	begin
		select * bulk collect into allcodes from country_codes order by length(code) desc;
		select * bulk collect into allroamerpartner from roamer_partner order by length(starting_imsi) desc;

		delete temp_du_inroamer_cdrs ;

		date_of_cdr := to_date('$DateToSummarize', 'dd/mm/yyyy') ;
		for inroamer_cdr in inroamer_cdr_list loop
			is_found := 0 ;
			if inroamer_cdr.imsi_number is not null and length(inroamer_cdr.imsi_number) > 0 then
				cdr_country_code := inroamer_cdr.imsi_number ;	
				if allroamerpartner.count > 0 then
					for i in allroamerpartner.first..allroamerpartner.last loop
						if substr(cdr_country_code, 0, length(allroamerpartner(i).starting_imsi)) = allroamerpartner(i).starting_imsi then
							cdr_country_code := allroamerpartner(i).country_code ;
							cdr_network_code := allroamerpartner(i).tap_code ;
							is_found := 1 ;
							exit ;
						end if ;
					end loop ;
				end if ;
			end if ;
			
			if is_found = 0 then
				cdr_network_code := 'Default' ;	
				cdr_country_code := inroamer_cdr.phone_number ;
				for i in allcodes.first..allcodes.last loop
					if substr(cdr_country_code, 0, length(allcodes(i).code)) = allcodes(i).code then
						cdr_country_code := allcodes(i).code ;
						exit ;
					end if ;
				end loop ;
			end if ;

			insert into temp_du_inroamer_cdrs
				(time_stamp, country_code, network_code, value, duration,imsi,stream_indicator)
			values
				(to_date(to_char(inroamer_cdr.time_stamp,'dd/mm/yyyy'),'dd/mm/yyyy'), cdr_country_code, cdr_network_code, inroamer_cdr.value, inroamer_cdr.duration,inroamer_cdr.imsi_number,'GSM') ;
		end loop ;

		for inroamer_gprs_cdr in inroamer_gprs_cdr_list loop
			begin
			cdr_network_code := 'Default' ;	
			select code into cdr_country_code from (select code from country_codes where inroamer_gprs_cdr.phone_number like code||'%' order by length(code) desc) where rownum<2 ;
			if SQL%ROWCOUNT = 0 then
				cdr_country_code := inroamer_gprs_cdr.phone_number ;
			end if ;
			exception
				when no_data_found then
				cdr_country_code := inroamer_gprs_cdr.phone_number ;
			end ;
			insert into temp_du_inroamer_cdrs
				(time_stamp, country_code, network_code, value, duration,stream_indicator)
			values
				(to_date(to_char(inroamer_gprs_cdr.time_stamp,'dd/mm/yyyy'),'dd/mm/yyyy'), cdr_country_code, cdr_network_code, inroamer_gprs_cdr.value, inroamer_gprs_cdr.upload_data_volume + inroamer_gprs_cdr.download_data_volume,'GPRS') ;
		end loop ;
	end ;
/
	insert into du_inroamer_cdr_summary 
		(time_stamp, country_code, network_code, total_count, total_duration, total_value,distinct_imsi_count,stream_indicator)
		select trunc(time_stamp), country_code, network_code, count(*), sum(duration), sum(value),count (distinct imsi),stream_indicator
			from temp_du_inroamer_cdrs
			group by trunc(time_stamp), country_code, network_code, stream_indicator ;
EOF

Status=$?
echo "" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo "" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo "" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo "DU Inroamer XDR Summary Run" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo "     --------------------------------------------" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo "Run Time       : `date`" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo "For Date       : [$DateToSummarize]" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo "For NetworkID  : [$NetworkID]" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
if [ $Status -ne 0 ]; then
	echo "Status         : Failure" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
	echo "            -----------------------------" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
	cat $RANGERHOME/LOG/TempDUInroamerCDRSummaryLog.tmp >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
	echo "" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
	echo "     ********************************************" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
	exit 1
fi
echo "Status         : Success" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo "            -----------------------------" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo -n "    Rows Created : " >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
cat $RANGERHOME/LOG/TempDUInroamerCDRSummaryLog.tmp | grep "created" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo -n "    Rows Deleted : " >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
cat $RANGERHOME/LOG/TempDUInroamerCDRSummaryLog.tmp | grep "deleted" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo "" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log
echo "     ********************************************" >> $RANGERHOME/LOG/DUInroamerCDRSummary.log

exit 0
