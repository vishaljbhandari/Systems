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
	rm -f $RANGERHOME/RangerData/Scheduler/DUPGWSummary"$NetworkID"
}
trap on_exit EXIT

function SummarizeTable
{
	sqlplus -s /nolog << EOF  >> $RANGERHOME/LOG/DUPGWSummary.log
		CONNECT_TO_SQL		
		whenever sqlerror exit 5 ;
		whenever oserror exit 6 ;

		set serveroutput on ;
		set feedback off ;
		declare
			is_match				number(2)		:= 0 ;
			amount					number(16,6)	:= 0 ;
			time_stamp				date			:= null ;
			credit_card_no			varchar2(24)	:= '' ;
			accountname				varchar2(40)	:= '' ;
			card_expiry_date		varchar2(8)		:= '' ;
			creditcard_holder_name	varchar2(64)	:= '' ;

		cursor pgw_data is select * from du_pgw_data where trunc(timestamp) = to_date('$DateToSummarize', 'dd/mm/yyyy') ;

		begin
			delete from du_pgw_summary
				where 
					trunc(time_stamp) = to_date('$DateToSummarize', 'dd/mm/yyyy')
					or trunc(time_stamp) <= sysdate - 
						nvl((select value from configurations where config_key =  'DU.PGW.SUMMARY.CLEANUP.DAYS'),15) ;
		
			for pgw_rows in pgw_data loop
				is_match := 0 ;
				begin
					select 1,time_stamp,card_no,account_name,expiry_date,card_holder_name,value
						into is_match,time_stamp,credit_card_no,accountname,card_expiry_date,creditcard_holder_name,amount
					from 
						du_pgw_summary
					where
						time_stamp = pgw_rows.timestamp and
						card_no = pgw_rows.card_no and
						account_name = pgw_rows.account_name and
						expiry_date = pgw_rows.expiry_date and
						card_holder_name = pgw_rows.card_holder_name ;
				exception
					when no_data_found then
						is_match := 0 ;
				end ;

				if is_match = 0 then
					insert into du_pgw_summary
						(time_stamp,card_no,account_name,expiry_date,card_holder_name,record_count,value)
					values
						(pgw_rows.timestamp,pgw_rows.card_no,pgw_rows.account_name,pgw_rows.expiry_date,pgw_rows.card_holder_name,1,pgw_rows.value) ;
				else
					update 
						du_pgw_summary
					set 
						 record_count = record_count + 1, value = value + pgw_rows.value
					where
						time_stamp = pgw_rows.timestamp and
						credit_card_no = pgw_rows.card_no and
						accountname = pgw_rows.account_name and
						card_expiry_date = pgw_rows.expiry_date and
						creditcard_holder_name = pgw_rows.card_holder_name ;
				end if ;
			end loop ;
			delete from du_pgw_data
				where 
					trunc(time_stamp) <= sysdate -	nvl((select value from configuration where id =  'DU.PGW.SUMMARY.CLEANUP.DAYS'),15) ;
		end ;
/
EOF
}

SummarizeTable
if [ $? -eq 0 ]; then
	echo "Summary successfully done on $DateToSummarize" >> $RANGERHOME/LOG/DUPGWSummary.log
else
	echo "Error in Summarisation !!! Please check $RANGERHOME/LOG/DUPGWSummary.log" >> $RANGERHOME/LOG/DUPGWSummary.log
fi
