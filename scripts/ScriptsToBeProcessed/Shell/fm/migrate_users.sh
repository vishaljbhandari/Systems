#! /bin/bash

LOG_FILE=migrate_users.log
. $RANGERHOME/sbin/rangerenv.sh

sqlplus -l -s $DB_USER/$DB_PASSWORD << EOF >> $LOG_FILE
SET SERVEROUTPUT ON ;
WHENEVER SQLERROR EXIT 5 ;
WHENEVER OSERROR EXIT 5 ;

execute MigrateUsers.Migrate('$RANGERHOME/scratch') ;

commit ;
quit ;
EOF

for name in `cat $RANGERHOME/scratch/user_names.txt`
do 
	mkdir -p $NIKIRACLIENT/data/user_folders/$name
done

if [ $? -ne 0 ] ; then
	exit 3
fi

