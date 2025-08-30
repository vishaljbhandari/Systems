#!/bin/bash

if [ -z "$RANGERHOME" ]
then
    echo "Messages"
    exit 1
fi

. anyshellscripfile.sh

LOGFILE=$PATH/filename.log

echo "[`date`] Script Started.." >>$LOGFILE 

sqlplus -s /nolog << EOF >> $LOGFILE 
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
set feedback on;
set serveroutput on;

@sql_file.sql

commit ;
quit ;
EOF

if [ $? -ne 0 ] ; then
     echo "[`date`] message" >> $LOGFILE
     exit 1
else
fi
