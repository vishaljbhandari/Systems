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

on_exit ()
{
        rm -f $RANGERHOME/RangerData/Scheduler/DUInternationalCDRSummary$NetworkID
}
trap on_exit EXIT

sqlplus -s /nolog << EOF > $RANGERHOME/LOG/TempDUInternationalCDRSummaryLog.tmp 2>&1
CONNECT_TO_SQL
        whenever sqlerror exit 5 ;
        whenever oserror exit 6 ;
        declare
                date_of_cdr date := null ;
                cdr_country_code varchar2(60) := null ;
                cursor international_cdr_list is select * from cdr where day_of_year = to_char(to_date(nvl('$DateToSummarize', to_char (sysdate - 1, 'dd/mm/yyyy')), 'dd/mm/yyyy'), 'ddd') 
                        and call_type = 3 and cdr_type in (1,2,3,4,5,8) ;
                type code_table is table of country_codes%rowtype index by pls_integer;
        allcodes code_table ;
        begin
                select * bulk collect into allcodes from country_codes order by length(code) desc;
                delete temp_du_international_cdrs ;
                date_of_cdr := to_date(nvl('$DateToSummarize', to_char (sysdate - 1, 'dd/mm/yyyy')), 'dd/mm/yyyy') ;
                for international_cdr in international_cdr_list loop
                        select decode (international_cdr.record_type, 1, international_cdr.called_number, 2, international_cdr.caller_number, 3, nvl(international_cdr.forwarded_number, '+0')) into cdr_country_code from dual ;
                        for i in allcodes.first..allcodes.last loop
                                if substr(cdr_country_code, 0, length(allcodes(i).code)) = allcodes(i).code then
                                        cdr_country_code := allcodes(i).code ;
                                        exit ;
                                end if ;
                        end loop ;

                        insert into temp_du_international_cdrs
                                (time_stamp, country_code, product_type, service_type, type_of_call, value, duration)
                        values
                                (to_date(to_char(international_cdr.time_stamp,'dd/mm/yyyy'),'dd/mm/yyyy'), cdr_country_code, international_cdr.cdr_type, 
                                decode(international_cdr.is_subscriberless, 'SS7', 8, international_cdr.service_type), 
                                decode(international_cdr.is_subscriberless, 'SS7',decode(international_cdr.record_type, 1,'FLO', 2,'FLI', 3,'FLF'), international_cdr.is_subscriberless)
                                , international_cdr.value, international_cdr.duration) ;
                end loop ;
        end ;
/
        insert into du_international_cdr_summary 
                (time_stamp, country_code, cdr_type, service_type, type_of_call, total_count, total_duration, total_value)
                select time_stamp, country_code, product_type, service_type, type_of_call, count(*), sum(duration), sum(value)
                        from temp_du_international_cdrs
                        group by time_stamp, country_code, product_type, service_type, type_of_call ;
        delete from du_international_cdr_summary where time_stamp < sysdate - (select nvl ((select value from configurations where config_key = 'DU.INTERNATIONAL.CDR.SUMMARY.CLEANUP.DAYS'), 15) from dual) ;
EOF

Status=$?
echo "" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo "" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo "" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo "DU International CDR Summary Run" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo "     --------------------------------------------" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo "Run Time       : `date`" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo "For Date       : [$DateToSummarize]" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo "For NetworkID  : [$NetworkID]" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
if [ $Status -ne 0 ]; then
        echo "Status         : Failure" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
        echo "            -----------------------------" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
        cat $RANGERHOME/LOG/TempDUInternationalCDRSummaryLog.tmp >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
        echo "" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
        echo "     ********************************************" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
        exit 1
fi
echo "Status         : Success" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo "            -----------------------------" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo -n "    Rows Created : " >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
cat $RANGERHOME/LOG/TempDUInternationalCDRSummaryLog.tmp | grep "rows created" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo -n "    Rows Deleted : " >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
cat $RANGERHOME/LOG/TempDUInternationalCDRSummaryLog.tmp | grep "rows deleted" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo "" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log
echo "     ********************************************" >> $RANGERHOME/LOG/DUInternationalCDRSummary.log

rm -f $RANGERHOME/LOG/TempDUInternationalCDRSummaryLog.tmp
exit 0
