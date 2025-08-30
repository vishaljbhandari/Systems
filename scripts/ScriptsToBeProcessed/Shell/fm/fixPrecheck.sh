#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh


if [ $DB_TYPE == "1" ]
then
DBVALUE=`sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD <<EOF
set heading off;
set feedback off;
select FIELD_ASSOCIATION from precheck_field_configs where RECORD_CONFIG_ID=4 and FIELD_ASSOCIATION like '5.%';
EOF`
if [ "$DBVALUE" == "" ]
then
    sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD <<EOF
    set feedback off;
    update precheck_field_configs set FIELD_ASSOCIATION='5.'||FIELD_ASSOCIATION where RECORD_CONFIG_ID=4;
    commit;
EOF
fi

else

DBVALUE=`clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
set heading off;
set feedback off;
select FIELD_ASSOCIATION from precheck_field_configs where RECORD_CONFIG_ID=4 and FIELD_ASSOCIATION like '5.%';
EOF`
if [ "$DBVALUE" == "" ]
then
    clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
    set feedback off;
    update precheck_field_configs set FIELD_ASSOCIATION='5.'||FIELD_ASSOCIATION where RECORD_CONFIG_ID=4;
    commit;
EOF
fi


fi