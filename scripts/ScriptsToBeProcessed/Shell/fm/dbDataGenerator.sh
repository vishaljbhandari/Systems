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
	fi

	tempVar=tempFiles/

AllTableList()
{
	Columns=(
		CLUSTER
		TABLE
		INDEX
		SEQUENCE
		SYNONYM
		TRIGGER
		PROCEDURE
		PACKAGE
		PACKAGE BODY
		TYPE
		TYPE BODY
		VIEW
		FUNCTION
		)
	>tempFiles/generateList.log
		
	echo "set pages 1000;" >>  $tempVar$column.sql
	for column in ${Columns[@]}
	do
		> $tempVar$column.sql
		echo "set pages 1000;" >>  $tempVar$column.sql
		echo "spool $tempVar$column.log;" >>  $tempVar$column.sql
		fetchSql="select OBJECT_NAME from user_objects where  OBJECT_TYPE ='$column' order by object_id;"

		echo $fetchSql >>  $tempVar$column.sql
		echo "spool off" >>  $tempVar$column.sql
	echo "exit" >>  $tempVar$column.sql
	$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @$tempVar$column.sql >> tempFiles/generateList.log 2>&1

	done

	DB_STATUS=$?

    for column in ${Columns[@]}
    do
		sed -i '/^--/ d' $tempVar$column.log
		sed -i '/^OBJECT_NAME/ d' $tempVar/$column.log
		sed -i '/^[0-9]/ d' $tempVar$column.log
		sed -i '/^\s*$/d' $tempVar$column.log
	done

echo
echo "DBSetup TableList generated."

}

UsageTableList()
{
	Columns=(
			LTE_IMSI
			IS_ATTEMPTED
			IPDR_TYPE
			ORDER_NUMBER
			ADJUSTMENT_TYPE
			)

		>tempFiles/usageTableList.log
		>tempFiles/usageTableList.sql
		echo "set pages 1000;" >> tempFiles/usageTableList.sql
		for column in ${Columns[@]}
	do
		fetchSql="select tname from tab WHERE TABTYPE !='VIEW' and tname in (select TABLE_NAME from user_Tab_columns where COLUMN_NAME ='$column') ;"

			echo $fetchSql >> tempFiles/usageTableList.sql

			done
			echo "exit" >> tempFiles/usageTableList.sql

			$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @tempFiles/usageTableList.sql >> tempFiles/usageTableList.log 2>&1
			DB_STATUS=$?

			sed -i '/^--/ d' tempFiles/usageTableList.log
			sed -i '/^TNAME/ d' tempFiles/usageTableList.log
			sed -i '/^[0-9]/ d' tempFiles/usageTableList.log
			sed -i '/^\s*$/d' tempFiles/usageTableList.log

	
	echo "UsageTableList Generated"
}

GetCoreTableList()
{
    AllTable=tempFiles/TABLE.log
	UsageTable=tempFiles/usageTableList.log

    >tempFiles/coreSchema.txt
	>tempFiles/usageSchema.txt
	>tempFiles/usageSchema.txt
	cat $UsageTable | uniq > tempFiles/usageSchema.txt
	cat $AllTable | uniq > tempFiles/dbSetupSchema.txt

	for table_name in `cat tempFiles/dbSetupSchema.txt`
		do
		val=`grep -i $table_name tempFiles/usageSchema.txt` >/dev/null
	if [[ "$val" != "$table_name" ]]; then
		echo $table_name >> tempFiles/coreSchema.txt
	fi
	done

   	echo "coreTableList generated"
}

GenerateCoreSchema()
{
	./SchemaGenerator.sh $User $Password tempFiles/coreSchema.txt
	
 	cp schema.xml fms_core_schema_v1.xml
 	cp seed_data.xml fms_core_seed_data.xml
	cp tableSpaceFile.xml fms_core_tableSpaceFile.xml
	rm schema.xml tableSpaceFile.xml

}

GenerateGDOSchema()
{
    ./SchemaGenerator.sh $User $Password tempFiles/usageSchema.txt
		    
    cp schema.xml fms_gdo_schema_v1.xml
 	cp seed_data.xml fms_gdo_seed_data.xml
	cp tableSpaceFile.xml fms_gdo_tableSpaceFile.xml
	rm schema.xml tableSpaceFile.xml
	sed -i 's/Nikira/NikiraGDO/ ' fms_gdo_schema_v1.xml
	sed -i 's/subex.nikira./com.subex.nikiragdo./ ' fms_gdo_schema_v1.xml

#  rm -rf tempFiles

}
AllTableList
UsageTableList
GetCoreTableList
GenerateCoreSchema
GenerateGDOSchema


