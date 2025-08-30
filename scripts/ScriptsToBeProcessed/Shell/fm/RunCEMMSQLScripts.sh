#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

processSQLScripts()
{
	sqlplus -s $PREVEA_DB_SETUP_USER/$PREVEA_DB_SETUP_PASSWORD @$WATIR_SERVER_HOME/data/sqlfile.sql <<EOF
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		commit;
EOF
}

processSQLScripts 
