#!/bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


cleanup()
{
	  if test -f $WATIR_SERVER_HOME/Scripts/tmp.sql; then
		  rm $WATIR_SERVER_HOME/Scripts/tmp.sql
	  fi		  
}

updateconfig()
{

if [ $DB_TYPE == "1" ]
then
	sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF

        set termout off
        set heading off
        set echo off
        @ $WATIR_SERVER_HOME/Scripts/tmp.sql;
        commit;
        quit

EOF

else
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
	set termout off
        set heading off
        set echo off
        @ $WATIR_SERVER_HOME/Scripts/tmp.sql;
        commit;
        quit

EOF
fi
}

main()
{
		cleanup
		sql=$*
		sql=${sql//QUOTE/\'}
		sql=${sql//SEMICOLON/\;}
		echo $sql > $WATIR_SERVER_HOME/Scripts/tmp.sql
	    updateconfig
}

main $*
