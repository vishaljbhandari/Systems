#/*******************************************************************************
#*  Copyright (c) Subex Limited 2014. All rights reserved.                      *
#*  The copyright to the computer program(s) herein is the property of Subex    *
#*  Limited. The program(s) may be used and/or copied with the written          *
#*  permission from Subex Limited or in accordance with the terms and           *
#*  conditions stipulated in the agreement/contract under which the program(s)  *
#*  have been supplied.                                                         *
#********************************************************************************/

#!/bin/bash

if [ -z "$RANGERHOME" ]
then
    echo "Set RANGERHOME and then run the migration"
    exit 1
fi

. $RANGERHOME/sbin/rangerenv.sh

LOGFILE=$RANGERHOME/LOG/migration_outroamer_gprs_vpmn_rates_install.log

echo "[`date`] Script Started.." >>$LOGFILE 

sqlplus -s /nolog << EOF >> $LOGFILE 
CONNECT_TO_SQL

whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

set feedback on;

@outroamer_gprs_vpmn_rates_install.sql

commit ;
quit ;
EOF

if [ $? -ne 0 ] ; then
     echo "[`date`] Migration failed..." >> $LOGFILE
     echo "[`date`] Migration failed..."
         echo "Please check the log at $LOGFILE "
         echo "********************************************************************************** " >> $LOGFILE
     exit 1
else
     mkdir $RANGERROOT/FMSData/VPMNLaderInputData
     echo "[`date`] Migration Success..." >> $LOGFILE
     echo "[`date`] Migration Success..."
         echo "Please check the log at $LOGFILE "
         echo "********************************************************************************** " >> $LOGFILE
fi
