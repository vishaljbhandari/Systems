#!/bin/bash
. $RUBY_UNIT_HOME/Scripts/configuration.sh
. $RANGERHOME/sbin/rangerenv.sh

TABLE=$1
COUNT=0
TIMEOUT=500

getTableCountSQL()
{
		$DB_QUERY <<EOF 
		set termout off;
		set heading off;
		set pages 1000;
		set lines 1000;
		select count(*) into :x from RELOAD_CONFIGURATIONS;
EOF
}

getTableCount()
{
	 getTableCountSQL $1 | tr -d "   [A-Za-z\.]\012"
}

nLoops=`expr $TIMEOUT / 2`

i=0
while test $i -lt $nLoops ; do
	curcount=`getTableCount $TABLE`
	if test $curcount -eq $COUNT; then
		exitValue=0
		break;
	else
		exitValue=0
		break;
	fi
	echo "Waiting for Reload To Complete"
	sleep 0.2
	i=`expr $i + 1`
done


if [ $exitValue == '0' ]
then
	echo "Table Count Passed"
else
	echo "Wait for table count failed"
	exit 1

fi
	
