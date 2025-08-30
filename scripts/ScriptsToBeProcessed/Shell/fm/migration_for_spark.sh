#!/bin/bash

NikiraDBUser=$DB_USER
NikiraDBPassword=$DB_PASSWORD

if [ -z $SPARK_DB_USER ]
then
	echo 'SPARK_DB_USER Not Set.'
	exit 1
fi

if [ -z $SPARK_DB_PASSWORD ]
then
	echo 'SPARK_DB_PASSWORD Not Set.'
	exit 1
fi

if [ -z $SPARK_ORACLE_SID ]
then
	echo 'SPARK_ORACLE_SID Not Set.'
	exit 1
fi

if [ -z $NIKIRA_ORACLE_SID ]
then
	echo 'NIKIRA_ORACLE_SID Not Set.'
	exit 1
fi

SparkDBUser=$SPARK_DB_USER
SparkDBPassword=$SPARK_DB_PASSWORD


function dropIndexes()
{
sqlplus -s $SparkDBUser/$SparkDBPassword@$SPARK_ORACLE_SID<<EOF
set feedback on ;

	drop index TABLE_COLUMN_SS1;
	drop index TABLE_COLUMN_SS2;

EOF
}

dropIndexes
