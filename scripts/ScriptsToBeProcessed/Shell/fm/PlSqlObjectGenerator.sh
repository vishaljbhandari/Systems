#! /bin/bash

. rangerenv.sh


User=$1
Password=$2
RunPath=`dirname $0`

if [ $# -lt 2 ]
then
	echo "Usage  $0 <DB_SUER> <DB_PASSWORD>  >"
    exit
fi

	cd $RunPath

	currentDir=$(pwd)
	if [ ! -d "$currentDir/tempFiles" ]; then
		mkdir $currentDir/tempFiles
	else
		rm -rf $currentDir/tempFiles/*
	fi
	
	 if [ ! -d "$currentDir/temp" ]; then               
		mkdir $currentDir/temp
	else
		rm -rf $currentDir/temp/*
        fi

	tempVar=tempFiles/
	temp=temp/

AllPlSqlObject()
{
	Columns=(
		PROCEDURE
		PACKAGE
		TYPE
		VIEW
		FUNCTION
		TRIGGER
		)
	>tempFiles/generateList.log

	for column in ${Columns[@]}
	do	
		> tempFiles/generateList.log
		> $tempVar$column.sql
		echo "set pages 1000;" >>  $tempVar$column.sql
		echo "set linesize 5000;" >> $tempVar$column.sql
		echo "set serveroutput off;" >> $tempVar$column.sql
		echo "set heading off; " >> $tempVar$column.sql
		echo "set feedback off;" >> $tempVar$column.sql
		echo "spool $tempVar$column.log;" >>  $tempVar$column.sql
		fetchSql="select OBJECT_NAME from user_objects where  OBJECT_TYPE ='$column' order by object_id;"

		echo $fetchSql >>  $tempVar$column.sql
		echo "spool off" >>  $tempVar$column.sql
	echo "exit" >>  $tempVar$column.sql
	$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @$tempVar$column.sql >> tempFiles/generateList.log 2>&1
	 
	for line in $(cat tempFiles/generateList.log)
        do
        > $temp$column.sql1
	echo "set pages 3000;" >>  $temp$column.sql1
	echo "set long 300000;" >>  $temp$column.sql1 
	echo "set linesize 5000;" >> $temp$column.sql1
	echo "set longchunksize 30000;" >> $temp$column.sql1
        echo "set serveroutput off;" >> $temp$column.sql1
        echo "set heading off; " >> $temp$column.sql1
        echo "set feedback off;" >> $temp$column.sql1
	echo "exec dbms_metadata.set_transform_param( DBMS_METADATA.SESSION_TRANSFORM, 'SQLTERMINATOR', TRUE );" >> $temp$column.sql1
	echo "spool $temp$column.log;" >>  $temp$column.sql1
        sql="select dbms_metadata.get_ddl('$column','$line') from dual;"
        echo $sql >>  $temp$column.sql1
        echo "spool off" >>  $temp$column.sql1
        echo "exit" >> $temp$column.sql1
        $SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @$temp$column.sql1 >> $temp$column.sql 2>&1
        done

	done

	DB_STATUS=$?


    for column in ${Columns[@]}
    do
		sed -i '/^--/ d' $tempVar$column.log
		sed -i '/^OBJECT_NAME/ d' $tempVar/$column.log
		sed -i '/^[0-9]/ d' $tempVar$column.log
		sed -i '/^\s*$/d' $tempVar$column.log
	done
	for column in ${Columns[@]}
	do
	sed -i '/^DBMS_METADATA.GET_DDL/ d' $temp$column.sql
	sed -i '/^--/ d' $temp$column.sql
	sed -i "s/"\"${User^^}"\".// " $temp$column.sql
	sed -i '/^\s*$/d' $temp$column.sql
	done
	


echo
echo "DBSetup PlSql Objects  generated."

}
AllPlSqlObject


