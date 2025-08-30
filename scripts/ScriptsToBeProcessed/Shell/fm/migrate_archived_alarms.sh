#! /bin/bash

LOG_FILE=migrate_archived_alarms.log
. $RANGERHOME/sbin/rangerenv.sh

sqlplus -l -s $DB_USER/$DB_PASSWORD << EOF >> $LOG_FILE
SET SERVEROUTPUT ON ;
WHENEVER SQLERROR EXIT 5 ;
WHENEVER OSERROR EXIT 5 ;

execute MigrateArchivedAlarms.Migrate('$RANGERHOME/scratch') ;

commit ;
quit ;
EOF

if [ $? -ne 0 ] ; then
	exit 3
fi

