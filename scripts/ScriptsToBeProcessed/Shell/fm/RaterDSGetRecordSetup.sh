#!/bin/bash

#> $RANGERHOME/LOG/etisalat-GSM-rater.log
#> $RANGERHOME/FMSData/RejectedCDRData/etisalat-GSM-rater.log

cp ./TestData/RaterDSGSMRecords.txt $RANGERHOME/FMSData/RaterCDRData/
touch $RANGERHOME/FMSData/RaterCDRData/success/RaterDSGSMRecords.txt
