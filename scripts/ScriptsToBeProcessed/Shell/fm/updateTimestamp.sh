#!/bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

updateTimestamp()
{
#       echo "update $table set $set_field=to_date('$set_value', 'DD-MM-YY HH24:MI:SS') where $where_field='$where_value';"
        
        if [ $DB_TYPE == "1" ]
	then
        sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF

        set termout off
        set heading off
        set echo off
        update $table set $set_field=to_date('$set_value', 'DD-MM-YY HH24:MI:SS') where $where_field='$where_value';
        commit;
        quit

EOF
else
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
	set termout off
        set heading off
        set echo off
        update $table set $set_field=to_date('$set_value', 'DD-MM-YY HH24:MI:SS') where $where_field='$where_value';
        commit;
        quit
EOF
fi

}

updateTimestampWithNoConditions()
{
#       echo "update $table set $set_field=to_date('$set_value', 'DD-MM-YY HH24:MI:SS') ;"
        if [ $DB_TYPE == "1" ]
	then
        sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF

        set termout off
        set heading off
        set echo off
        update $table set $set_field=to_date('$set_value', 'DD-MM-YY HH24:MI:SS') ;
        commit;
        quit

EOF

else
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
        set termout off
        set heading off
        set echo off
        update $table set $set_field=to_date('$set_value', 'DD-MM-YY HH24:MI:SS') ;
        commit;
        quit

EOF


fi

}

if [ $# -eq 5 ]
then
        table=$1
        set_field=$2
        set_value=$3
        where_field=$4
        where_value=$5

        updateTimestamp
else
        table=$1
        set_field=$2
        set_value=$3

        updateTimestampWithNoConditions
fi
