#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


tablename=$1


case $tablename in
	RULES ) id="KEY"; WHERE_CONDITION="name='$2'";;
	TAGS ) id="ID"; WHERE_CONDITION="name='$2' and category='$3'";;
	SOURCE ) id="ID"; WHERE_CONDITION="description='$2'";;
	AUDIT_LOG_EVENT_CODES ) id="ID"; WHERE_CONDITION="description='$2'";;
	TRANSLATIONS ) id="KEY"; WHERE_CONDITION="value='$2'";;
	RANGER_GROUPS ) id="ID"; WHERE_CONDITION="name='$2'";;
	RANGER_USERS ) id="ID"; WHERE_CONDITION="name='$2'";;
	PRECHECK_LISTS ) id="ID"; WHERE_CONDITION="name='$2'";;
	FP_ENTITIES) id="ID"; WHERE_CONDITION="DESCRIPTION='$2'";;
    ALARMS) id="ID"; WHERE_CONDITION="reference_value='$2'";;
    RECORD_CONFIGS) id="ID"; WHERE_CONDITION="DESCRIPTION='$2'";;
    FIELD_CATEGORIES) id="ID"; WHERE_CONDITION="category like '%HOTLIST%' and name='$2'";;
esac

TMPDIR="$WATIR_SERVER_HOME/Scripts/tmp"
if [ -d $TMPDIR ]
then
   sleep 0
else
   mkdir $TMPDIR
fi

rm $TMPDIR/EntityID.lst
#echo "select id from $tablename where $WHERE_CONDITION;" 

if [ $DB_TYPE == "1" ]
then
sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF

set termout off ;
set heading off ;
set echo off ;

SPOOL $TMPDIR/EntityID.lst ;
select $id from $tablename where $WHERE_CONDITION;
SPOOL OFF ;

quit ;

EOF

else

clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved." | grep -v "off" | grep -v "SPOOL" | grep -iv "SELECT" | grep -v "quit" | grep -v "SET" | grep -v ";"

set termout off ;
set heading off ;
set echo off ;

SPOOL $WATIR_SERVER_HOME/Scripts/tmp/EntityID.lst ;
select $id from $tablename where $WHERE_CONDITION;
SPOOL OFF ;
quit ;

EOF

fi

EntityId=`grep "[0-9][0-9]*" $WATIR_SERVER_HOME/Scripts/tmp/EntityID.lst | tr -s ' ' | tr -d ' '`

