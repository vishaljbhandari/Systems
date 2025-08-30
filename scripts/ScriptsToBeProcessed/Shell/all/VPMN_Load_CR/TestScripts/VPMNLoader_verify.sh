#! /bin/bash
                                                                                                                 
#/*******************************************************************************                                 
#*  Copyright (c) Subex Limited 2014. All rights reserved.                      *                                 
#*  The copyright to the computer program(s) herein is the property of Subex    *                                 
#*  Limited. The program(s) may be used and/or copied with the written          *                                 
#*  permission from Subex Limited or in accordance with the terms and           *                                 
#*  conditions stipulated in the agreement/contract under which the program(s)  *                                 
#*  have been supplied.                                                         *                                 
#********************************************************************************/ 

if [ -z "$RANGERHOME" ]; then
    echo "RANGERHOME is not set"
    exit 1
fi

if [ $# -lt 1 ]
then
        echo "Usage: scriptlauncher $0 SampleDataFile "
        exit 2
fi

SampleData=$1

if [ ! -f $SampleData ]
then
        echo "File Does not Exist: $SampleData"
        exit 3
else
        sed '/^[ \t]*$/d' < $SampleData > $SampleData.tmp
        mv $SampleData.tmp  $SampleData
        echo "$SampleData" | grep "\." > /dev/null 2>&1
        if [ $? -ne 0 ]
        then
                mv $SampleData $SampleData.dat
        fi
fi

countoffilerecordswithheader=`cat $SampleData | wc -l`
countoffilerecords=`expr $countoffilerecordswithheader - 1`
if [ $countoffilerecords -eq 0 ]
then
    echo "Data File is empty"
    exit 4
fi

sqlplus -s /nolog << eof
CONNECT_TO_SQL

set serveroutput off;
set feedback off;
set lines 120;
set colsep ,
set pages 0;
set heading off;
spool vpmnloader.tmp
select count(*) from OUTROAMER_GPRS_VPMN_RATES;
spool off;
quit;
eof

sed '/^[ \t]*$/d' < vpmnloader.tmp > vpmnloader.tmp.tmp
mv vpmnloader.tmp.tmp vpmnloader.tmp

countofdbrecords=`cat vpmnloader.tmp`
if [ $countofdbrecords -eq 0 ]
then
    echo "DB Table Empty, Loading Failed"
    exit 5
fi

if [ $countofdbrecords -ne $countoffilerecords ]
then
    echo "All Data records are not loaded to database"
    exit 6
fi
rm vpmnloader.tmp
echo "Verification Successfull..."
