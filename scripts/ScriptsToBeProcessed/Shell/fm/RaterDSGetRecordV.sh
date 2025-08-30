#!/bin/bash

OutFile="$RANGERHOME/FMSData/DataRecord/Process/RaterDSGSMRecords.txt"
LogFile="$RANGERHOME/LOG/etisalat-GSM-rater.log"

Lines=`cat $OutFile | sed -n "$="`

if [ x"$Lines" != "x4" ]; then
	exit 1
fi
