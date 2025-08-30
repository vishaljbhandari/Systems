#!/bin/bash

rm  -f $RANGERHOME/FMSData/RaterGPRSCDRData/success/*
rm  -f $RANGERHOME/FMSData/RaterGPRSCDRData/*

rm -f $RANGERHOME/FMSData/DataRecord/Process/*
rm -f $RANGERHOME/FMSData/DataRecord/Process/success/*

cp TestData/getrecorddatagprs.txt $RANGERHOME/FMSData/RaterGPRSCDRData/
touch $RANGERHOME/FMSData/RaterGPRSCDRData/success/getrecorddatagprs.txt
