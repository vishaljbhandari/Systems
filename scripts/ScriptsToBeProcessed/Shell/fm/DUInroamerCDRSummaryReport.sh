#################################################################################
#  Copyright (c) Subex Systems Limited 2007. All rights reserved.               #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Systems Limited. The program (s) may be used and/or copied with the written  #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

#! /bin/bash

NetworkID=$1
StartDate=$2
EndDate=$3

on_exit ()
{
        rm -f $RANGERHOME/RangerData/Scheduler/DUInroamerCDRSummaryReportPID$NetworkID
}
trap on_exit EXIT 

. $RANGERHOME/sbin/rangerenv.sh

sqlplus -s /nolog << EOF > mailID3.txt
CONNECT_TO_SQL
set heading off
set wrap off
set newpage none
set termout off
set feedback off
whenever sqlerror exit 4 ;
whenever oserror exit 5 ;
set lines 1000;
select value from configurations where config_key='DU.DAILY.INTERNATIONAL.XDR.TREND.REPORT.MAIL.ID' ;
quit;
EOF

if [ $? -ne 0 ]; then
        logger "Unable To Complete SQL Procedure [Fetching E-Mail ID's Failed !]"
        exit 1
fi

echo "Daily Inroamer XDR Trend." > $RANGERHOME/share/Ranger/DUInroamerCDRSummaryReport.csv
echo "Inroamer XDR Usage Summary Between $StartDate And $EndDate" >> $RANGERHOME/share/Ranger/DUInroamerCDRSummaryReport.csv
echo -e "\n\n"
echo "Date,Country,Network (TapCode),CDR Count,Total Duaration (hh:mm:ss)/Volume (MB),Total Value,Distinct IMSI Count,Stream Indicator" >> $RANGERHOME/share/Ranger/DUInroamerCDRSummaryReport.csv

sqlplus -s /nolog << EOF >> $RANGERHOME/share/Ranger/DUInroamerCDRSummaryReport.csv
CONNECT_TO_SQL
        whenever sqlerror exit 5 ;
        whenever oserror exit 6 ;
        set heading off ;
        set feedback off ;
        set linesize 5000 ;
        set pagesize 5000 ; 
        select to_char(time_stamp, 'dd/mm/yyyy') || ',' || (select description from country_codes where code = country_code) || ' (' || country_code || '),' || 
                network_code || ',' || total_count || ',' || 
                decode(stream_indicator,'GSM',to_char (floor(TRUNC(total_duration/60/60, '009'))) || ':' || trim(to_char(trunc(mod(total_duration, 3600)/60), '09')) || ':' || trim(to_char(mod(mod(total_duration, 3600), 60),'09')),'GPRS',total_duration) || ',' ||
                total_value ||','||  distinct_imsi_count ||','|| stream_indicator from du_inroamer_cdr_summary 
                where 
                trunc(time_stamp) between trunc(to_date('$StartDate', 'dd/mm/yyyy')) and trunc(to_date('$EndDate', 'dd/mm/yyyy'))
                order by time_stamp ;
EOF

if [ $? -ne 0 ]; then
        logger "Report Generation Failed For Daily Inroamer XDR Trend !"
        exit 2
fi

MAILID=`cat mailID3.txt | tail -1`
rm -f mailID3.txt
uuencode $RANGERHOME/share/Ranger/DUInroamerCDRSummaryReport.csv DUInroamerCDRSummaryReport.csv | mailx -s "DU Daily Inroamer XDR Trend Report" "$MAILID"
