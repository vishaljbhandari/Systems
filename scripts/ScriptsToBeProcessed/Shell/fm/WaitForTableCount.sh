#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

TABLE=$1
COUNT=$2
TIMEOUT=200

getTableCountSQL()
{
	if [ $DB_TYPE == "1" ]
	then
	sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD <<EOF
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		spool alias.txt
		select count(*) from $1;
		spool off;
EOF
	else
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
			set termout off;
			set heading off;
			set pages 1000;
			set lines 1000;
			spool alias.txt 
			select count(*) from $1;
			spool off;
EOF

fi
}

getTableCount()
{
	 getTableCountSQL $1 | tr -d "   [A-Za-z\.]\012"
}

nLoops=`expr $TIMEOUT / 2`

i=0
while test $i -lt $nLoops ; do
	curcount1=`getTableCount $TABLE`
	curcount=`cat alias.txt | grep -v "SQL"  | tr -s ""` #this line is needed only while running AT for db2 database
	if test $curcount -ge $COUNT ; then
		exitValue=0
		break;
	else
		exitValue=1
	fi
	echo "sleeping "
	sleep 0.2
	i=`expr $i + 1`

    echo $curcount  > $WATIR_SERVER_HOME/Scripts/tmp/RecordCount.txt
    echo $TABLE  >> $WATIR_SERVER_HOME/Scripts/tmp/RecordCount.txt

done


if [ $exitValue == '0' ]
then
	echo "Table Count Passed"
else
	echo "Wait for table count failed"
	exit 1

fi
	
