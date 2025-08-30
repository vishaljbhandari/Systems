#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


processSQLScripts()
{
	if [ $DB_TYPE == "1" ]
then 
	sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD @$WATIR_SERVER_HOME/data/sqlfile.sql <<EOF
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		commit;
EOF

else
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME @$WATIR_SERVER_HOME/data/sqlfile.sql <<EOF
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		commit;
EOF
fi

}


processSQLScripts 
