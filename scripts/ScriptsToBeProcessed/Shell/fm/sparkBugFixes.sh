#!/bin/bash

NikiraDBUser=$1
NikiraDBPassword=$2
NikiraOracleSid=$3
SparkDBUser=$4
SparkDBPasswd=$5
SparkOracleSid=$6
SparkDbLink=$7
FMDBLink=$8



if [ $# -lt 2 ]
then
	echo "Usage: $0 <NikiraDBUser> <NikiraDBPassword>"
	exit 1
fi

if [ -z $SparkDBUser ]
then
	echo 'SPARK_DB_USER Not Set. Synonym Creation is Not Possible'
	exit 1
fi

if [ -z $SparkDBPasswd ]
then
    echo 'SPARK_DB_PASSWORD  Not Set. Synonym Creation is Not Possible'
	exit 1

fi

if [ -z $SparkOracleSid ]
then
	echo 'SPARK_ORACLE_SID  Not Set. Synonym Creation is Not Possible'
	exit 1

fi

if [ -z $NikiraOracleSid ]
then
	echo 'NIKIRA_ORACLE_SID  Not Set. Synonym Creation is Not Possible'
	exit 1
fi

if [ -z $FMDBLink ]
then
    echo 'NIKIRA_DB_LINK Not Set. Synonym Creation is Not Possible'
    exit 1
fi

if [ -z $SparkDbLink ]
then
    echo 'SPARK_DB_LINK Not Set. Synonym Creation is Not Possible'
    exit 1
fi

SparkDBUser=$SparkDBUser
SparkDBPassword=$SparkDBPasswd
SparkToNikDBLink=$FMDBLink
NikToSparkDBLink=$SparkDbLink

Failed=0
LogFile=sparkBugFixes.log
> $LogFile

function createsynonym()
{
	tablename=$1
	synonymname=nik_$1

	if [ $# -eq 2 ]
	then
		synonymname=$2
	fi

	sqlplus -s $SparkDBUser/$SparkDBPassword@$SparkOracleSid << EOF
	set feedback off ;

	CREATE OR REPLACE SYNONYM $synonymname FOR $NikiraDBUser.$tablename@$FMDBLink ;

EOF

	if [ $? -ne 0 ] ;then
		echo "Synonym creation error for $synonymname" >> $LogFile
		exit 1
	fi;

	if [ $NikiraOracleSid = $SparkOracleSid ];then
		AccessUser=$SparkDBUser
	else
		AccessUser='public'
	fi;


	if [ $? -ne 0 ] ;then
		echo "Granting error for $synonymname" >> $LogFile
		exit 1
	fi
}

#createsynonym list_details list_details
#createsynonym hotlist_groups_suspect_values hotlist_groups_suspect_values
#createsynonym auto_suspect_nums auto_suspect_nums
#createsynonym suspect_values suspect_values
#createsynonym hotlist_per_key hotlist_per_key
#createsynonym map_for_srs map_for_srs

echo "Granting privielge in spark schema" >> $LogFile
sqlplus -s -l $NikiraDBUser/$NikiraDBPassword@$NikiraOracleSid >> $LogFile <<EOF
whenever sqlerror exit 2 ;
@grant_privelage_spark_on_nikira.sql
quit;
EOF

if [ $? -ne 0 ];then
	echo "Granting privileges failed for spark schema" >> $LogFile
	Failed=1
fi

echo "Granting privielge in nikira schema" >> $LogFile
sqlplus -s -l $SparkDBUser/$SparkDBPassword@$SparkOracleSid >> $LogFile <<EOF
whenever sqlerror exit 2 ;
whenever oserror exit 1 ;
set feedback off ;
@grant_nikira.sql
host rm grant_nikira.sql
EOF

if [ $? -ne 0 ];then
	echo "Granting prvileges failed for nikira schema" >> $LogFile
	Failed=1
fi

echo "Recompiling invalid objects" >> $LogFile
sqlplus -s -l $SparkDBUser/$SparkDBPassword@$SparkOracleSid >> $LogFile <<EOF
@recompile_invalid
EOF


if [ $Failed -eq 1 ];then
	echo "Failed" >> $LogFile
	exit 1
else
	echo "Successful" >> $LogFile
fi
