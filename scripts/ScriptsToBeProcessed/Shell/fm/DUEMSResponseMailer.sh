#!/bin/bash

. rangerenv.sh

InputDir="$RANGER5_4HOME/RangerData/$1/Responses"

if [ ! -d "$RANGER5_4HOME/RangerData/$1/Responses" ]; then
	echo "Usage : DUEMSResponseMailer.sh <Module Directory name>"
	echo "      - ./DUEMSResponseMailer.sh DataSourcePIData"
	echo "      - ./DUEMSResponseMailer.sh DataSourceCRMIData"
	exit 10
fi

sqlplus -s /nolog << EOF > mailID3.txt
	CONNECT_TO_SQL
	set feedback off
	set heading off
	set serveroutput off
	whenever sqlerror exit 4 ;
	whenever oserror exit 5 ;
	select value from configuration where id='DU.FAILED.EMS.NOTIFICATION.MAIL.ID' ;
	quit;
EOF

MAILID=`cat mailID3.txt | tail -1`
rm -f mailID3.txt

for FailureFile in `ls $InputDir/*.FAILURE`
do
	NewName=`echo "$FailureFile" | sed 's/\.FAILURE//'`
	mv $FailureFile $NewName
	echo "Transaction Failure Notification ..." > $RANGERHOME/tmp/notificationmessage.txt
	echo "" >> $RANGERHOME/tmp/notificationmessage.txt
	echo " Transaction with the source system failed with a non-zero error code." >> $RANGERHOME/tmp/notificationmessage.txt
	echo "" >> $RANGERHOME/tmp/notificationmessage.txt
	echo " Listing is the XML message used as request." >> $RANGERHOME/tmp/notificationmessage.txt
	echo "----------------------------------------------------------------" >> $RANGERHOME/tmp/notificationmessage.txt
	cat $NewName >> $RANGERHOME/tmp/notificationmessage.txt
	echo "----------------------------------------------------------------" >> $RANGERHOME/tmp/notificationmessage.txt
	echo "" >> $RANGERHOME/tmp/notificationmessage.txt
	echo " File Details : $NewName" >> $RANGERHOME/tmp/notificationmessage.txt
	echo "----------------------------------------------------------------" >> $RANGERHOME/tmp/notificationmessage.txt

	uuencode $RANGERHOME/tmp/notificationmessage.txt notificationmessage.txt | mailx -s "DU Failed EMS Transaction." "$MAILID"
	rm -f $RANGERHOME/tmp/notificationmessage.txt
done
