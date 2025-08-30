#! /bin/bash
. /nikira/NIKIRAROOT/sbin/rangerenv.sh

cd /nikira/NIKIRAROOT/tmp/

REPORT_DATE=`sqlplus -s /nolog << EOF
				CONNECT_TO_SQL
				set heading off ;
				set feedback off ;
				select to_char (sysdate - 1, 'dd/mm/yyyy') from dual ;
EOF`
echo $REPORT_DATE > /nikira/NIKIRAROOT/tmp/date_check

### Inroamer CDR Summary report ###
/nikira/NIKIRAROOT/bin/scriptlauncher.sh -r "/nikira/NIKIRAROOT/bin/DUInroamerCDRSummaryReport.sh" DUInroamerCDRSummaryReportPID999 999 $REPORT_DATE $REPORT_DATE #RangerCronJob
sleep 1800
/nikira/NIKIRAROOT/bin/scriptlauncher.sh -k DUInroamerCDRSummaryReportPID999 #RangerCronJob

### Oldest File Summary report ###
/nikira/NIKIRAROOT/bin/scriptlauncher.sh -r "/nikira/NIKIRAROOT/bin/DUOldestFileProcessedForDay.sh" DUOldestFileProcessedForDayPID999 999 $REPORT_DATE $REPORT_DATE #RangerCronJob
sleep 1800
/nikira/NIKIRAROOT/bin/scriptlauncher.sh -k DUOldestFileProcessedForDayPID999 #RangerCronJob
