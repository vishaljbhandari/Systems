#!/bin/bash

NikiraDBUser=$1
NikiraDBPassword=$2

if [ $# -lt 2 ]
then
	echo "Usage: $0 <NikiraDBUser> <NikiraDBPassword>"
	exit 1
fi

if [ -z $SPARK_DB_USER ]
then
	read -p "Enter SPARK_DB_USER : " SPARK_DB_USER
fi

if [ -z $SPARK_DB_PASSWORD ]
then
	read -p "Enter SPARK_DB_PASSWORD : " SPARK_DB_PASSWORD
fi

if [ -z $SPARK_ORACLE_SID ]
then
	read -p "Enter SPARK_ORACLE_SID : " SPARK_ORACLE_SID
fi

if [ -z $NIKIRA_ORACLE_SID ]
then
	read -p "Enter NIKIRA_ORACLE_SID : " NIKIRA_ORACLE_SID
fi

if [ -z $NIKIRA_DB_LINK ]
then
    echo 'NIKIRA_DB_LINK Not Set. Synonym Creation is Not Possible'
    exit 1
fi

if [ -z $SPARK_DB_LINK ]
then
    echo 'SPARK_DB_LINK Not Set. Synonym Creation is Not Possible'
    exit 1
fi

SparkDBUser=$SPARK_DB_USER
SparkDBPassword=$SPARK_DB_PASSWORD
SparkToNikDBLink=$NIKIRA_DB_LINK
NikToSparkDBLink=$SPARK_DB_LINK

Failed=0
LogFile=spark_migration.log
> $LogFile

function createsynonym()
{
	tablename=$1
	synonymname=nik_$1

	if [ $# -eq 2 ]
	then
		synonymname=$2
	fi

	sqlplus -s $SparkDBUser/$SparkDBPassword@$SPARK_ORACLE_SID << EOF
	set feedback off ;

	CREATE OR REPLACE SYNONYM $synonymname FOR $NikiraDBUser.$tablename@$NIKIRA_DB_LINK ;

EOF

	if [ $? -ne 0 ] ;then
		echo "Synonym creation error for $synonymname"
		exit 1
	fi;

	if [ $NIKIRA_ORACLE_SID = $SPARK_ORACLE_SID ];then
		AccessUser=$SparkDBUser
	else
		AccessUser='public'
	fi;

	sqlplus -s $NikiraDBUser/$NikiraDBPassword@$NIKIRA_ORACLE_SID<< EOF
	whenever sqlerror exit 1 ;
	set feedback off ;

	GRANT ALL on $NikiraDBUser.$tablename to $AccessUser ;
	delete from spark_synonym_maps where SYNONYM_NAME='$synonymname' or TABLE_NAME='$tablename' ;
	insert into spark_synonym_maps(TABLE_NAME, SYNONYM_NAME) values ('$tablename','$synonymname') ;
	commit ;

EOF

	if [ $? -ne 0 ] ;then
		echo "Granting error for $synonymname"
		exit 1
	fi
}

createsynonym list_details list_details
createsynonym hotlist_groups_suspect_values hotlist_groups_suspect_values
createsynonym auto_suspect_nums auto_suspect_nums
createsynonym suspect_values suspect_values
createsynonym hotlist_per_key hotlist_per_key

echo "Granting privielge in spark schema"
sqlplus -s -l $NikiraDBUser/$NikiraDBPassword@$NIKIRA_ORACLE_SID >> $LogFile <<EOF
whenever sqlerror exit 2 ;
@grant_privelage_spark_on_nikira.sql
quit;
EOF

if [ $? -ne 0 ];then
	echo "Granting privileges failed for spark schema"
	Failed=1
fi

echo "Granting privielge in nikira schema"
sqlplus -s -l $SparkDBUser/$SparkDBPassword@$SPARK_ORACLE_SID >> $LogFile <<EOF
whenever sqlerror exit 2 ;
whenever oserror exit 1 ;
set feedback off ;
@grant_nikira.sql
host rm grant_nikira.sql
EOF

if [ $? -ne 0 ];then
	echo "Granting prvileges failed for nikira schema"
	Failed=1
fi

echo "Recompiling invalid objects"
sqlplus -s -l $SparkDBUser/$SparkDBPassword@$SPARK_ORACLE_SID >> $LogFile <<EOF
@recompile_invalid
EOF

sqlplus -s -l $NikiraDBUser/$NikiraDBPassword@$NIKIRA_ORACLE_SID >> $LogFile <<EOF
@recompile_invalid
EOF

if [ $Failed -eq 1 ];then
	echo "Spark Migration Failed"
	exit 1
else
	echo "Spark Migration Successful"
fi
