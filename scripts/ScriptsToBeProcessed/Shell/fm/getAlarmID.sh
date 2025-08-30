#!/bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


alarmid()
{
        VALUE=$1

if [ $DB_TYPE == "1" ]
then
      sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF
        spool $HOME/alarmid
        set termout off
        set heading off
        set echo off
        select id from alarms where REFERENCE_VALUE=$VALUE;
        commit;
        quit
EOF

else

	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."
spool $HOME/alarmid
set termout off
set heading off
set echo off
select id from alarms where REFERENCE_VALUE=$VALUE;
commit;
quit
EOF

fi

}


get_alarm_id()
{
	alarm_id=`grep "[0-9][0-9]*" $TMPDIR/alarmid.lst | tr -s ' ' | tr -d ' '`
	echo $alarm_id
}

main()
{
        alarmid $1
        get_alarm_id
}

main $1
