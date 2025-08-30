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
	rm -f $RANGERHOME/RangerData/Scheduler/DUCDRSummary$NetworkID
}
trap on_exit EXIT

sqlplus /nolog << EOF > $RANGERHOME/LOG/TempDUCDRSummaryLog.tmp 2>&1
CONNECT_TO_SQL
	whenever sqlerror exit 5 ;
	whenever oserror exit 6 ;

	--delete from du_daily_cdr_summary where trunc(time_stamp) = trunc(to_date('$DateToSummarize', 'dd/mm/yyyy')) ;

	insert into du_daily_cdr_summary (time_stamp, 
  product_type, service_type, call_type, type_of_call, total_count, total_duration, total_value,stream_indicator)
		select
		to_date(to_char(time_stamp, 'dd/mm/yyyy'), 'dd/mm/yyyy'),
		cdr_type, decode(is_subscriberless, 'SS7', 8, service_type) service_type, call_type, 
    decode(is_subscriberless, 'SS7',decode(record_type, 1,'FLO', 2,'FLI', 3,'FLF'), is_subscriberless) type_of_call,
    count(*), sum(duration), sum(value),decode(is_subscriberless, 'SS7', is_subscriberless, 'GSM')
		from cdr
		where day_of_year = to_char(to_date('$DateToSummarize', 'dd/mm/yyyy'), 'ddd') and cdr_type in (1,2,3,4,5,8)
		group by to_date(to_char(time_stamp, 'dd/mm/yyyy'), 'dd/mm/yyyy'),cdr_type, service_type, call_type, is_subscriberless, record_type ;


	insert into du_daily_cdr_summary (time_stamp, product_type, service_type, total_count, total_duration, total_value,stream_indicator, call_type, type_of_call)
		select
		to_date(to_char(time_stamp, 'dd/mm/yyyy'), 'dd/mm/yyyy'),
		cdr_type, service_type, count(*), sum(UPLOAD_DATA_VOLUME + DOWNLOAD_DATA_VOLUME), sum(value),'GPRS',9, '--'
		from gprs_cdr
		where day_of_year = to_char(to_date(nvl('$DateToSummarize', to_char (sysdate - 1, 'dd/mm/yyyy')), 'dd/mm/yyyy'), 'ddd') and cdr_type in (1,2,3,4,5,8)
		group by to_date(to_char(time_stamp, 'dd/mm/yyyy'), 'dd/mm/yyyy'),cdr_type, service_type ;

  insert into du_daily_cdr_summary (time_stamp, 
  product_type, service_type, call_type, type_of_call, total_count, total_duration, total_value,stream_indicator)
		select
		to_date(to_char(netflow_date, 'dd/mm/yyyy'), 'dd/mm/yyyy') time_stamp,
		3 cdr_type, 9 service_type, 9 call_type, '--' type_of_call, count(*), sum(total_upload_volume), sum(total_value_cost),'NETFLOW'
    from netflow
    where day_of_year = to_char(to_date(nvl('$DateToSummarize', to_char (sysdate - 1, 'dd/mm/yyyy')), 'dd/mm/yyyy'), 'ddd') 
		group by to_date(to_char(netflow_date, 'dd/mm/yyyy'), 'dd/mm/yyyy') ;

	delete from du_daily_cdr_summary where time_stamp < sysdate - (select nvl ((select value from configuration where id = 'DU.CDR.SUMMARY.CLEANUP.DAYS'), 15) from dual) ;
EOF

Status=$?
echo "" >> $RANGERHOME/LOG/DUCDRSummary.log
echo "" >> $RANGERHOME/LOG/DUCDRSummary.log
echo "" >> $RANGERHOME/LOG/DUCDRSummary.log
echo "DU CDR Summary Run" >> $RANGERHOME/LOG/DUCDRSummary.log
echo "     --------------------------------------------" >> $RANGERHOME/LOG/DUCDRSummary.log
echo "Run Time       : `date`" >> $RANGERHOME/LOG/DUCDRSummary.log
echo "For Date       : [$DateToSummarize]" >> $RANGERHOME/LOG/DUCDRSummary.log
echo "For NetworkID  : [$NetworkID]" >> $RANGERHOME/LOG/DUCDRSummary.log
if [ $Status -ne 0 ]; then
	echo "Status         : Failure" >> $RANGERHOME/LOG/DUCDRSummary.log
	echo "            -----------------------------" >> $RANGERHOME/LOG/DUCDRSummary.log
	cat $RANGERHOME/LOG/TempDUCDRSummaryLog.tmp >> $RANGERHOME/LOG/DUCDRSummary.log
	echo "" >> $RANGERHOME/LOG/DUCDRSummary.log
	echo "     ********************************************" >> $RANGERHOME/LOG/DUCDRSummary.log
	exit 1
fi
echo "Status         : Success" >> $RANGERHOME/LOG/DUCDRSummary.log
echo "            -----------------------------" >> $RANGERHOME/LOG/DUCDRSummary.log
echo -n "    GSM/Fixedline Rows Created  : " >> $RANGERHOME/LOG/DUCDRSummary.log
cat $RANGERHOME/LOG/TempDUCDRSummaryLog.tmp | grep "created" | head -1 >> $RANGERHOME/LOG/DUCDRSummary.log
echo -n "    GPRS Rows Created : " >> $RANGERHOME/LOG/DUCDRSummary.log
cat $RANGERHOME/LOG/TempDUCDRSummaryLog.tmp | grep "created" | tail -1 >> $RANGERHOME/LOG/DUCDRSummary.log
echo -n "    Rows Deleted      : " >> $RANGERHOME/LOG/DUCDRSummary.log
cat $RANGERHOME/LOG/TempDUCDRSummaryLog.tmp | grep "deleted" | head -1 >> $RANGERHOME/LOG/DUCDRSummary.log
echo "" >> $RANGERHOME/LOG/DUCDRSummary.log
echo "     ********************************************" >> $RANGERHOME/LOG/DUCDRSummary.log

rm -f $RANGERHOME/LOG/TempDUCDRSummaryLog.tmp

exit 0
