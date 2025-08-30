#!/bin/bash
#set -x

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


TABLE=$1
COUNT=$2
ENTITY=$3
PARTITIONID=$4
SUBPARTITIONID=$5
TIMEOUT=60

getPartitionCountSQL()
{

if [ $DB_TYPE == "1" ]
then 
sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD <<EOF
    set termout off;
    set heading off;
    set pages 1000;
    set lines 1000;
spool alias_123.txt;
select count(*) from $TABLE $ENTITY ("P${PARTITIONID}") ;
spool off
EOF

else

clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
set termout off;
set heading off;
set pages 1000;
set lines 1000;
spool alias_123.txt;
select count(*) from $TABLE $ENTITY ("P${PARTITIONID}") ;
spool off
EOF

fi

}

getSubPartitionCountSQL()
{
if [ $DB_TYPE == "1" ]
then
sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD <<EOF
    set termout off;
    set heading off;
    set pages 1000;
    set lines 1000;
spool alias_456.txt;
select count(*) from $TABLE $ENTITY ("P${PARTITIONID}_SP_${SUBPARTITIONID}") ;
spool off;
EOF

else
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
set termout off;
set heading off;
set pages 1000;
set lines 1000;
spool alias_456.txt;
select count(*) from $TABLE $ENTITY ("P${PARTITIONID}_SP_${SUBPARTITIONID}") ;
spool off;
EOF

fi

}

nLoops=`expr $TIMEOUT / 2`
if [ $PARTITIONID -lt 100 ]
then
	PARTITIONID="0${PARTITIONID}"
fi

i=0
while test $i -lt $nLoops ; do
    case $# in
        4)  curcount=`getPartitionCountSQL | tr -d "   [A-Za-z\.]\012"`;
	    curcount1=`cat alias_123.txt | grep -v "SQL"  | tr -s ""`; #this line is needed only while running AT for db2 database								 
;;
        5)  curcount=`getSubPartitionCountSQL | tr -d "   [A-Za-z\.]\012"`;
            curcount2=`cat alias_456.txt | grep -v "SQL"  | tr -s ""`; #this line is needed only while running AT for db2 database
    ;;
    esac
    if test \( $curcount1 -ge $COUNT \) -o \( $curcount2 -ge $COUNT \) -o \($curcount -ge $COUNT \)  ; then
        exitValue=0
        break;
    else
        exitValue=1
    fi
    sleep 2
    i=`expr $i + 1`
done


if [ $exitValue == 0 ]
then
    echo "Table Count Passed"
else
    echo "Wait for table count failed"
    exit 1

fi
