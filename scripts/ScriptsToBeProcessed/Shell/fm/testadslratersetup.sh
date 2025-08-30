#!/bin/bash

rm  -f $RANGERHOME/FMSData/RaterCDRData/success/*
rm  -f $RANGERHOME/FMSData/RaterCDRData/*

rm -f $RANGERHOME/FMSData/DataRecord/Process/*
rm -f $RANGERHOME/FMSData/DataRecord/Process/success/*

cp TestData/testgetrecordadsl.txt $RANGERHOME/FMSData/RaterCDRData/
touch $RANGERHOME/FMSData/RaterCDRData/success/testgetrecordadsl.txt
