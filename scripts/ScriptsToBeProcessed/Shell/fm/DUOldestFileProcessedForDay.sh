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
        rm -f $RANGERHOME/RangerData/Scheduler/DUOldestFileProcessedForDayPID$NetworkID
}
trap on_exit EXIT 

. $RANGERHOME/sbin/rangerenv.sh

sqlplus -s /nolog << EOF > mailID3.txt
CONNECT_TO_SQL
set feedback off
set heading off
set wrap off
set newpage none
set termout off
set lines 1000;
whenever sqlerror exit 4 ;
whenever oserror exit 5 ;
select value from configurations where config_key ='DU.DAILY.OLDEST.FILE.PROCESSED.MAIL.ID' ;
quit;
EOF

if [ $? -ne 0 ]; then
        logger "Unable To Complete SQL Procedure [Fetching E-Mail ID's Failed !]"
        exit 1
fi

echo "Oldest File Processed For A Day." > $RANGERHOME/share/Ranger/DUOldestFileProcessedReport.csv
echo "Oldest File Processed In $StartDate And $EndDate" >> $RANGERHOME/share/Ranger/DUOldestFileProcessedReport.csv
echo -e "\n\n"
echo "Processed Date,File Name,DataSource Name,File TimeStamp,Captured Sequence" >> $RANGERHOME/share/Ranger/DUOldestFileProcessedReport.csv

sqlplus -s /nolog << EOF >> $RANGERHOME/share/Ranger/DUOldestFileProcessedReport.csv
CONNECT_TO_SQL
        whenever sqlerror exit 5 ;
        whenever oserror exit 6 ;
        set heading off ;
        set feedback off ;
        set linesize 5000 ;
        set pagesize 5000 ; 
        select to_char(process_date, 'dd/mm/yyyy') || ',' || file_name || ',' || (select description from du_source_description where source = ds_id) 
                                || ',' || to_char(file_timestamp, 'yyyy/dd/mm hh24:mi:ss') || ',' || sequance_number
                from du_oldest_file_processed 
                where 
                trunc(process_date) between trunc(to_date('$StartDate', 'dd/mm/yyyy')) and trunc(to_date('$EndDate', 'dd/mm/yyyy'))
                order by process_date desc;
EOF

if [ $? -ne 0 ]; then
        logger "Report Generation Failed For Oldest File Processed !"
        exit 2
fi

MAILID=`cat mailID3.txt | tail -1`
rm -f mailID3.txt
uuencode $RANGERHOME/share/Ranger/DUOldestFileProcessedReport.csv DUOldestFileProcessedReport.csv | mailx -s "DU Daily Oldest File Processed Report" "$MAILID"
