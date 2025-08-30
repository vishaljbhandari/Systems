#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


task_name=$1

if [ $# -ge 2 ]
then
        p1=$2
fi
if [ $# -ge 3 ]
then
        p2=$3
fi

TMPDIR="$WATIR_SERVER_HOME/Scripts/tmp"
if [ -d $TMPDIR ]
then
        sleep 0
else
        mkdir $TMPDIR
fi
tmpsqloutput=".tmpsqloutput"

if [ $DB_TYPE == "1" ]
then
sqlplus -s /NOLOG << EOF > $tmpsqloutput  2>&1
    CONNECT $DB_SETUP_USER/$DB_SETUP_PASSWORD
    @$WATIR_SERVER_HOME/Scripts/getTaskID.sql
    WHENEVER OSERROR  EXIT 6 ;
    WHENEVER SQLERROR EXIT 5 ;
        SPOOL $TMPDIR/configentry.lst
        select GETTASKID('$task_name', '$p1', '$p2') from dual ;
        SPOOL OFF ;
EOF

else
clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF > $tmpsqloutput  2>&1 | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved." | grep -v ";"

set termout off ;
set heading off ;
set echo off ;
@/home/testdb2/Watir/Scripts/getTaskID.sql

SPOOL $WATIR_SERVER_HOME/Scripts/tmp/configentry.lst ;
select GETTASKID('$task_name', '$p1', '$p2') from dual ;
SPOOL OFF ;
EOF


fi

configEntry=`grep "[0-9][0-9]*" $TMPDIR/configentry.lst|grep -v GETTASKID| tr -s ' ' | tr -d ' '`
echo $configEntry
