#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


rm $WATIR_SERVER_HOME/data/*.sql
cp $WATIR_SERVER_HOME/Scripts/$1 $WATIR_SERVER_HOME/data/sqlfile.sql

processSQLScripts()
{
	sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD @$WATIR_SERVER_HOME/data/sqlfile.sql <<EOF
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		commit;
EOF

	sqlplus -s $SPARK_DB_USER/$SPARK_DB_USER @$WATIR_SERVER_HOME/data/sqlfile.sql <<EOF
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		commit;
EOF

}


processSQLScripts 
