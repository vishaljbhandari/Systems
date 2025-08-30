#/*******************************************************************************
# *  Copyright (c) Subex Limited 2013. All rights reserved.                     *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Limited. The program(s) may be used and/or copied with the written         *
# *  permission from Subex Limited or in accordance with the terms and          *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
# *******************************************************************************/
#!/bin/bash

if [ -z "$RANGERHOME" ]
then
    echo "Set RANGERHOME and then run the migration"
    exit 1
fi

RANGER_PROFILE_FILE=$RANGERHOME/sbin/rangerenv.sh

if [ ! -f $RANGER_PROFILE_FILE ]
then
        echo "The mentioned Ranger Env file $RANGER_PROFILE_FILE not found.";
        echo "Please mention the correct path for rangerenv.sh";
        exit 2 
fi

. $RANGER_PROFILE_FILE

if [ -z "$DB_USER" ]
then
    echo "Set ROC_FM USER and then run the migration"
    exit 3
fi

if [ -z "$DB_PASSWORD" ]
then
    echo "Set ROC_FM DB PASSWORD and then run the migration"
    exit 4
fi

LOGFILE=$RANGERHOME/LOG/Migration_for_new_alarm_view.log

echo "[`date`] Script Started.." >>$LOGFILE

sqlplus -s  $DB_USER/$DB_PASSWORD   << EOF >> $LOGFILE 
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 6 ;
set feedback on ;
set serveroutput on ;

@alarm_view_new.sql

commit;
quit;
EOF

if [ $? -eq 0 ]
        then
        echo "Successfully Done!" >>$LOGFILE;
        echo "Successfully Done! Please check the log file @ $LOGFILE";
else
        echo "Error in migration!" >>$LOGFILE;
        echo "Error in migration!" Please check the log file @ $LOGFILE;
fi
