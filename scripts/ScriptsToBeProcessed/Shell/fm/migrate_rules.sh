#! /bin/bash

LOG_FILE=migrate_rules.log
. $RANGERHOME/sbin/rangerenv.sh

sqlplus -l -s $DB_USER/$DB_PASSWORD << EOF >> $LOG_FILE
SET SERVEROUTPUT ON ;
WHENEVER SQLERROR EXIT 5 ;
WHENEVER OSERROR EXIT 5 ;

execute MigrateRules.Migrate('$RANGERHOME/scratch') ;
execute MigrateRechargeRules.Migrate('$RANGERHOME/scratch') ;
execute MigrateGPRSRules.Migrate('$RANGERHOME/scratch') ;
execute MigratePrecheckRules.Migrate('$RANGERHOME/scratch') ;

commit ;
quit ;
EOF

if [ $? -ne 0 ] ; then
	exit 3
fi

