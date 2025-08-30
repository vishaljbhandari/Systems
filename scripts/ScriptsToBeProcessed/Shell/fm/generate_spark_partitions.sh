#!/bin/bash

TEMP_FILE=excel_data.csv.tmp
HEADER_INFO_FILE=header_info.txt
DATA_FILE=data_info.txt
INFO_FILE=INFO.txt
>$INFO_FILE

rm -f $TEMP_FILE $HEADER_INFO_FILE $DATA_FILE

function GetColumnListFromFieldConfigs ()
{
	sqlplus -s $DB_USER/$DB_PASSWORD@$ORACLE_SID << EOF  > /dev/null
	whenever sqlerror exit 5
	set feedback off 
	set verify off
	set pagesize 0
	set lines 2000
	set trimspool on ;
	set term off
	set termout off
	set heading off
	set echo off
	set serveroutput off
	spool $DATA_FILE
	select 'V|'|| upper(database_field)||'|'||S.DATA_TYPE||'|0' from field_configs fc, SPARK_DATA_TYPES_MAP S where record_config_id = (select id from record_configs where upper(tname) = upper('$INPUT_TABLE_NAME') and rownum < 2) and database_field is not null and fc.DATA_TYPE = S.FC_DATA_TYPE order by position  ;
	spool off ;
EOF
	if [ -s $DATA_FILE ]
	then
		echo > /dev/null
		#echo "V|RECORDTYPE|STRING|0" >> $DATA_FILE
		#echo "V|RECORD_NUMBER|INT|0" >> $DATA_FILE
		#echo "V|STS_ID|INT|0" >> $DATA_FILE
		#echo "V|SFL_ID|INT|0" >> $DATA_FILE

	else
		echo " Table Name $INPUT_TABLE_NAME does not exist in RECORD_CONFIGS TABLE... No Data File Generated"
		exit 2
	fi
	cat $DATA_FILE > $DATA_FILE.tmp
	cat $DATA_FILE.tmp | tr "|" "\t"  >> EXCEL_INPUT_"$MAIN_TABLE_NAME"_"$DateTime".csv
}
function GenerateHeaderDataInfo ()
{
	DateTime=`date +%d%m%Y_%H%M%S`
	cat EXCEL_INPUT.csv | tr "\t" "|" | sed -e 's/|$//g' | grep -v ^$  > $TEMP_FILE

	gawk -F"|" -v InfoFile=$HEADER_INFO_FILE -v DataFile=$DATA_FILE -v SQ="'" '{
				if ($1 == "H") 
					print $2"="SQ$3SQ >> InfoFile	
				if ($1 == "V") 
					print $0 >> DataFile
			}' $TEMP_FILE

	current_path=`pwd`
	source $current_path/$HEADER_INFO_FILE
	cp EXCEL_INPUT.csv EXCEL_INPUT_"$MAIN_TABLE_NAME"_"$DateTime".csv
	if [ ! -f $DATA_FILE ]
	then
		echo "-----------------Field Info is not provided in Excel.... Taking the field info from Database--------------"
		echo
		INPUT_TABLE_NAME=$MAIN_TABLE_NAME
		GetColumnListFromFieldConfigs
	fi
}

function GenerateTableStructure ()
{
		if [ ! -d $HOME/OracleDBFFiles ]
		then
#			echo "NOOOOOOOOOOOOO"
			mkdir -p $HOME/OracleDBFFiles
			chmod 777 $HOME/OracleDBFFiles
		fi
		OracleDataFilePath="$HOME/OracleDBFFiles"
		NO_OF_DB_FIELDS=`gawk 'END{print NR}' $DATA_FILE`
		INDEX_FILE=$MAIN_TABLE_NAME"_INDEX_DDL.sql"
		DROP_INDEX_FILE="DROP_"$INDEX_FILE
		DROP_TABLE_FILE="DROP_"$MAIN_TABLE_NAME"_DDL.sql"
		DDL_TEMP_FILE=DDL.sql

		log_name=`echo $INDEX_FILE|gawk -F '.' '{print$1}'`
		log_name="$log_name.log"

		EXPORT_FILE=export_file.txt
		base_log_name=`basename $log_name`

		rm -f $INDEX_FILE $DROP_INDEX_FILE $DROP_TABLE_FILE $DDL_TEMP_FILE


		
gawk -F"|" 	-v DDLFile=$DDL_TEMP_FILE \
		-v INFO_FILE=$INFO_FILE \
		-v EXPORT_FILE=$EXPORT_FILE\
		-v SQ="'" \
		-v NO_OF_PHYSICAL_TABLES=$RETENTION_IN_DAYS \
		-v RETENTION_IN_DAYS=$RETENTION_IN_DAYS \
		-v LOAD_PER_DAY=$LOAD_PER_DAY \
		-v RECORD_CONFIG_ID=$RECORD_CONFIG_ID \
		-v LOAD_TYPE=$LOAD_TYPE \
		-v MAIN_TABLE_NAME=$MAIN_TABLE_NAME \
		-v NO_OF_DB_FIELDS=$NO_OF_DB_FIELDS \
		-v MAX_RECORDS_PER_PARTITION=10 \
		-v DROP_INDEX_FILE=$DROP_INDEX_FILE \
		-v INDEX_FILE=$INDEX_FILE \
		-v DROP_TABLE_FILE="$DROP_TABLE_FILE" \
		-v BUFFER_PERCENTAGE=10     \
		-v INDEX_BUFFER_PERCENTAGE=10 \
		-v MILLION=1000000 \
		-v ADDITIONAL_RETENTION_DAYS=2 \
		-v AVG_RECORD_LENGTH=400 \
		-v INDEX_RECORD_LENGTH=50 \
		-v MAX_SIZE_OF_DATAFILE_IN_GB=62 \
		-v TABLESPACE_LOCAL_FILE=TS_TABLESPACE_LOCAL_$MAIN_TABLE_NAME.sql \
		-v TABLESPACE_PROD_FILE=TS_TABLESPACE_PROD_$MAIN_TABLE_NAME.sql \
		-v TABLESPACE_INDEX_LOCAL_FILE=TS_TABLESPACE_INDEX_LOCAL_$MAIN_TABLE_NAME.sql \
		-v TABLESPACE_INDEX_PROD_FILE=TS_TABLESPACE_INDEX_PROD_$MAIN_TABLE_NAME.sql \
		-v ASM_DISK_SPACE_NAME="+DATA_USG" \
		-v TABLE_DDL_FILE=$MAIN_TABLE_NAME"_DDL".sql \
		-v DROP_TABLE_DDL_FILE="DROP_"$MAIN_TABLE_NAME"_DDL".sql \
		-v ORACLE_DATA_FILE_PATH="$OracleDataFilePath" \
		-v MAX_SIZE_OF_TABLE_SPACE=300 \
		'
			function PrintTS (ts_name,Env,DDLCommand,MaxSize,Object)
			{
				if (Object == "INDEX")
				{
					BlockSize = "BLOCKSIZE 32K"
					TableSpaceName = ts_name"IND_32K"
				}
				else
				{
					BlockSize = "" 
					TableSpaceName = ts_name
				}
				
					
				if (DDLCommand == "CREATE" && Env == "PROD")
				{
					print "CREATE TABLESPACE " TableSpaceName " DATAFILE " SQ ASM_DISK_SPACE_NAME SQ " SIZE 16M AUTOEXTEND ON MAXSIZE " MaxSize "G EXTENT MANAGEMENT LOCAL UNIFORM SIZE 1M SEGMENT SPACE MANAGEMENT AUTO " BlockSize " ;" > TABLESPACE_PROD_FILE
				}

				if (DDLCommand == "CREATE" && Env == "LOCAL")
				{
					print "CREATE TABLESPACE " TableSpaceName " DATAFILE " SQ ORACLE_DATA_FILE_PATH"/"TableSpaceName".dbf" SQ " SIZE 200K AUTOEXTEND ON MAXSIZE 10G EXTENT MANAGEMENT LOCAL UNIFORM SIZE 100K SEGMENT SPACE MANAGEMENT AUTO ;" > TABLESPACE_LOCAL_FILE
				}

				if (DDLCommand == "ALTER" && Env == "PROD")
				{
					print "ALTER TABLESPACE " TableSpaceName " ADD DATAFILE " SQ ASM_DISK_SPACE_NAME SQ " SIZE 16M AUTOEXTEND ON MAXSIZE " MaxSize "G ;" > TABLESPACE_PROD_FILE
				}
			}
		#function round(x,   ival, aval, fraction)
		#{
		#   ival = int(x)    # integer part, int() truncates

		#   # see if fractional part
		#   if (ival == x)   # no fraction
		#	  return ival   # ensure no decimals

		#   if (x < 0) {
		#	  aval = -x     # absolute value
		#	  ival = int(aval)
		#	  fraction = aval - ival
		#	  if (fraction >= .5)
		#		 return int(x) - 1   # -2.5 --> -3
		#	  else
		#		 return int(x)       # -2.3 --> -2
		#   } else {
		#	  fraction = x - ival
		#	  if (fraction >= .5)
		#		 return ival + 1
		#	  else
		#		 return ival
		#   }
		#}
			function round(Val)
			{
				TempVal = int (Val) ;
				FinalVal = (Val == TempVal ? Val : TempVal +1) ;
				return FinalVal ;
			}
			function GetNoOfPartitions (Partition)
			{
				NoOfPartitions = 0  ;
				if (Partition == 0)
				{
					return 0
				}	
				NoOfPartitions = LOAD_PER_DAY/MAX_RECORDS_PER_PARTITION
				NoOfPartitions = round(NoOfPartitions) ;
				return NoOfPartitions ;
			}
			function GetDataType (InputDataType)
			{
				return DataTypeArray[InputDataType] ;
			}
			function GetTablespaceSize (Object,ToTNum)
			{
				TS_Size = 0
				if (Object == "TABLE")
				{
						printf("(%-30s) %s * %s * (%s + %s) * %s\n","Calculation Formula For Table:","MILLION","LOAD_PER_DAY","RETENTION_IN_DAYS","ADDITIONAL_RETENTION_DAYS","AVG_RECORD_LENGTH") > INFO_FILE
						printf("( %s * %s * (%s + %s) * %s)\n",MILLION,LOAD_PER_DAY,RETENTION_IN_DAYS,ADDITIONAL_RETENTION_DAYS,AVG_RECORD_LENGTH) > INFO_FILE
						printf("(Buffer Percentage for Table is) %s%s\n",BUFFER_PERCENTAGE,"%") > INFO_FILE
						print "" > INFO_FILE

						TS_Size = MILLION * LOAD_PER_DAY * (RETENTION_IN_DAYS + ADDITIONAL_RETENTION_DAYS) * AVG_RECORD_LENGTH ;
						TS_Size = TS_Size + (TS_Size * (BUFFER_PERCENTAGE/100))
						TS_Size = TS_Size/(1024*1024*1024) 
						TS_Size = round(TS_Size) ;
				
						printf("Tablespace Size for %s is %s (GB) \n",MAIN_TABLE_NAME,TS_Size) > INFO_FILE;
						print "" > INFO_FILE
						print "" > INFO_FILE
						return TS_Size ;
				}
				else
				{
						printf("(%-30s) %s * %s * %s * (%s + %s) * %s\n","Calculation Formula For Index:","No Of Indexes","MILLION","LOAD_PER_DAY","RETENTION_IN_DAYS","ADDITIONAL_RETENTION_DAYS","INDEX_RECORD_LENGTH") > INFO_FILE
						printf("(%s * %s * %s * (%s + %s) * %s)\n",ToTNum,MILLION,LOAD_PER_DAY,RETENTION_IN_DAYS,ADDITIONAL_RETENTION_DAYS,INDEX_RECORD_LENGTH) > INFO_FILE
						printf("(Buffer Percentage for Index is) %s%s\n",INDEX_BUFFER_PERCENTAGE,"%") > INFO_FILE
						print "" > INFO_FILE
						TS_Size = ToTNum * MILLION * LOAD_PER_DAY * (RETENTION_IN_DAYS + ADDITIONAL_RETENTION_DAYS) * INDEX_RECORD_LENGTH ;
						TS_Size = TS_Size + (TS_Size * (INDEX_BUFFER_PERCENTAGE/100))
						TS_Size = TS_Size/(1024*1024*1024) 
						TS_Size = round(TS_Size) ;
						printf("Tablespace Size for %s is %s (GB) \n","Indexes",TS_Size) > INFO_FILE;
						print "" > INFO_FILE
						return TS_Size ;
				}
			}
			function GetTSNameForNPTable()
			{
				#print "GetTSNameForNPTable ......... NoOfTablesPerTSForNP ==" NoOfTablesPerTSForNP
				#print "GetTSNameForNPTable ......... NoOfTSForNP ==" NoOfTSForNP
				k = 1 ;
				j = 1 ;
				for ( i = 1 ; i <= NO_OF_PHYSICAL_TABLES ; i++)
				{
					if (k > NoOfTSForNP)
					{
						k = 1 
					}
					if ( j <= NoOfTablesPerTSForNP)
					{
						TSName = sprintf("TS_%s_%03s",RECORD_CONFIG_ID,k) ;
						TSMapForNP[i] =  TSName ;
						j+=1 ;
					}
					else
					{
						j=1
						TSName = sprintf("TS_%s_%03s",RECORD_CONFIG_ID,k) ;
						k+=1 
						TSMapForNP[i] =  TSName ;
						j+=1 ;
					}
				}
			}
			function GetTSNameForNPTableNew()
			{
				k = 1 ;
				j = 1 ;
				for ( i = 1 ; i <= NO_OF_PHYSICAL_TABLES ; i++)
				{
					TSName = sprintf("TS_%s_%03s",RECORD_CONFIG_ID,k) ;
					TSNameForIndex = sprintf("TS_%s_%03s",RECORD_CONFIG_ID,j) ;
					TSMapForNPNew[i] =  TSName ;
					TSMapForNPNewForIndex[i] = TSNameForIndex ;
					printf("%d:%s;",i,TSName) > EXPORT_FILE;
					#if ( k == NoOfTSForNP )
					if ( k == NoOfTablespaces )
					{
						k = 0 ;
					}
					if ( j == NoOfIndexTablespaces )
					{
						j = 0 ;
					}
						k+=1 ;
						j+=1 ;
				}
			}
			function PrintIndexStructure(TabName,Partition,TabNum,IndexNum,IndexName,IndexField)
			{
				TName = sprintf("%s_%03s",TabName,TabNum) ;
				print "DROP INDEX " IndexName " ;" > DROP_INDEX_FILE	
				if (Partition ==1 )
				{
					print "CREATE INDEX " IndexName " ON "TName"(" IndexField ") INITRANS 4 PARALLEL 4"
                       print "LOCAL"
                       print "("
                       for ( p = 1 ; p <= OraclePartitions ; p++)
                       {
							TS_Name = sprintf("TS_%s_%03sIND_32K",RECORD_CONFIG_ID,p) ;
                           if ( p == OraclePartitions)
                               ParEndChar=""
                           else
                               ParEndChar=","
                           print "PARTITION P" p " TABLESPACE " TS_Name ParEndChar 
                       }

				print ") ;" 
				}
				else
				{
					#printf ("CREATE INDEX %s ON %s(%s) TABLESPACE %sIND_32K ;\n",IndexName,TName,IndexField,TSMapForNP[TabNum]) ;
					printf ("CREATE INDEX %s ON %s(%s) TABLESPACE %sIND_32K ;\n",IndexName,TName,IndexField,TSMapForNPNewForIndex[TabNum]) ;
				}
			}
			function PrintTableStructure(TabName,Partition,TabNum)
			{
				TableName = sprintf("%s_%03s",TabName,TabNum) ;
				print "DROP TABLE " TableName "; "  > DROP_TABLE_DDL_FILE
				print "CREATE TABLE " TableName "(" 
				 system("cat DDL.sql") 
				if (Partition ==1 )
				{
                       print ") STORAGE (BUFFER_POOL RECYCLE)" 
                       print "INITRANS 4 PARALLEL 4" 
                       print "PARTITION BY RANGE (HOUR_OF_DAY)"
                       print "(" 
                       for ( p = 1 ; p <= OraclePartitions ; p++)
                       {
							TS_Name = sprintf("TS_%s_%03s",RECORD_CONFIG_ID,p) ;
                           if ( p == OraclePartitions)
                               ParEndChar=""
                           else
                               ParEndChar=","
                           print "PARTITION P" p " VALUES LESS THAN (" p+1 ") TABLESPACE " TS_Name ParEndChar 
                       }

				print ") ;" 
				}
				else
				{
					#print ") TABLESPACE " TSMapForNP[TabNum] " ;" ;
					print ") TABLESPACE " TSMapForNPNew[TabNum] " ;" ;
				}
			}
			function GetTablespaceScript(NoOfTS,TotalSize,Object)
			{
				if(is_partition ==1 )
				{
				SizeOfEachTS = TotalSize/NoOfTS ;
				SizeOfEachTS = round(SizeOfEachTS) ;
				}
			
				else
				{
					MaxSizeOfTS = MAX_SIZE_OF_TABLE_SPACE
					if (TotalSize > MaxSizeOfTS)
					{
							NoOfTS = round(TotalSize/MaxSizeOfTS)
							SizeOfEachTS = round(TotalSize/NoOfTS)
							NoOfTablesPerTS = int(NO_OF_PHYSICAL_TABLES/NoOfTS)
							#print "ELSE ......." TotalSize,MaxSizeOfTS,NoOfTS, round(NoOfTS),NO_OF_PHYSICAL_TABLES,SizeOfEachTS,NoOfTablesPerTS
					}
					else
					{
						NoOfTS =1 
						SizeOfEachTS =1 
						NoOfTablesPerTS = NO_OF_PHYSICAL_TABLES
					}
				}	
				
				
				NoOfDataFilesPerTS = SizeOfEachTS/MAX_SIZE_OF_DATAFILE_IN_GB
				NoOfDataFilesPerTS = round(NoOfDataFilesPerTS) ;
				Total = NoOfDataFilesPerTS * MAX_SIZE_OF_DATAFILE_IN_GB
				#SizeOfLastDataFile = Total - SizeOfEachTS ;
				SizeOfLastDataFile = SizeOfEachTS - ((NoOfDataFilesPerTS -1) * MAX_SIZE_OF_DATAFILE_IN_GB) ;
				if (SizeOfLastDataFile < 10)
					SizeOfLastDataFile = 10

				#print "GetTablespaceScript ........." NoOfTS, TotalSize ,SizeOfEachTS , NoOfDataFilesPerTS , Total , SizeOfLastDataFile
				printf("(%s) %-30s ==>  %s \n",Object,"Max No Of Records Per Partition",MAX_RECORDS_PER_PARTITION) > INFO_FILE;
				printf("(%s) %-30s ==>  %s \n",Object,"Total Tabespace Size",TotalSize) > INFO_FILE;
				printf("(%s) %-30s ==>  %s \n",Object,"Max Size Of DataFile",MAX_SIZE_OF_DATAFILE_IN_GB)  > INFO_FILE;
				if (Object == "INDEX")
					NoOfIndexTablespaces = NoOfTS
				else
					NoOfTablespaces = NoOfTS
				printf("(%s) %-30s ==>  %s \n",Object,"No Of Tablespaces",NoOfTS) > INFO_FILE;
				printf("(%s) %-30s ==>  %s \n",Object,"Size Of Each Tablespace",SizeOfEachTS) > INFO_FILE;
				printf("(%s) %-30s ==>  %s \n",Object,"No Of DataFiles Per TS",NoOfDataFilesPerTS) > INFO_FILE;
				printf("(%s) %-30s ==>  %s \n",Object,"Total Size Obtained with all DataFiles",Total) > INFO_FILE;
				printf("(%s) %-30s ==>  %s \n",Object,"Size Of Last DataFile",SizeOfLastDataFile) > INFO_FILE;
				
				
				for ( i = 1 ; i <= NoOfTS ; i++ )
				{
					TS_Name = sprintf("TS_%s_%03s",RECORD_CONFIG_ID,i) ;
					PrintTS(TS_Name,"PROD","CREATE",MAX_SIZE_OF_DATAFILE_IN_GB,Object)
					PrintTS(TS_Name,"LOCAL","CREATE",MAX_SIZE_OF_DATAFILE_IN_GB,Object)
					for ( j = 2 ; j <= NoOfDataFilesPerTS ; j++ )
					{
						if ( j < NoOfDataFilesPerTS)
						{
							PrintTS(TS_Name,"PROD","ALTER",MAX_SIZE_OF_DATAFILE_IN_GB,Object)  
						}
						else
						{
							PrintTS(TS_Name,"PROD","ALTER",SizeOfLastDataFile,Object)  
						}
					}
						print "---------------------------------------------" > TABLESPACE_LOCAL_FILE
						print "---------------------------------------------" > TABLESPACE_PROD_FILE
				}
			}
			function GetNoOfTablesPerTSForNP (TotalSize)
			{
					MaxSizeOfTS = MAX_SIZE_OF_TABLE_SPACE
					if (TotalSize > MaxSizeOfTS)
					{
							NoOfTS = round(TotalSize/MaxSizeOfTS)
							SizeOfEachTS = round(TotalSize/NoOfTS)
							NoOfTablesPerTS = int(NO_OF_PHYSICAL_TABLES/NoOfTS)
							return NoOfTablesPerTS ;
					}
					else 
						return NO_OF_PHYSICAL_TABLES ;
			}
			function GetNoOfTSForNP (TotalSize)
			{
					MaxSizeOfTS = MAX_SIZE_OF_TABLE_SPACE
					if (TotalSize > MaxSizeOfTS)
					{
						NoOfTS = round(TotalSize/MaxSizeOfTS)
						return NoOfTS ;
					}
					else
						return 1;
			}
			BEGIN \
			{
				InputDataTypes="BIGINT,NUMBER(19),INT,NUMBER(10),FLOAT,NUMBER(19),STRING,VARCHAR2(255),TIMESTAMP,TIMESTAMP(6),BOOL,CHAR(1)"
				NoOfDataTypes = split(InputDataTypes,xls,",") ;
				NoOfDataTypes = NoOfDataTypes/2 ;
				j = 0 ;
				k = 1 ;
				print "----------------------------------------------------"> INFO_FILE
				printf ("DATA TYPES Info:\n") > INFO_FILE
				print "----------------------------------------------------"> INFO_FILE
				for (i = 1 ; i <= NoOfDataTypes ; i++)
				{
					j = j +2 ;
					#printf("k = %s ; j = %s \n",k,j) ;
					DataType = xls[k] ;
					DataTypeValue = xls[j] ;
					printf("%-15s ==>  %s\n",DataType,DataTypeValue) > INFO_FILE
					DataTypeArray[DataType] = DataTypeValue ;
					k = k + 2 ;
				}
				if (LOAD_PER_DAY >= 200)
				{
					is_partition = 1 ;
					LoadType = "Direct Data Load"
				}
				else
				{
					is_partition = 0 ;
					LoadType = "ROCFM Conventional Load"
				}

				print "" > INFO_FILE

				print "----------------------------------------------------"> INFO_FILE
				printf ("Load Info:\n") > INFO_FILE
				print "----------------------------------------------------"> INFO_FILE
				printf("%-15s ==>  %s\n","Table Name",MAIN_TABLE_NAME) > INFO_FILE
				printf("%-15s ==>  %s\n","Load Per Day",LOAD_PER_DAY) > INFO_FILE
				printf("%-15s ==>  %s\n","Load Type",LoadType) > INFO_FILE
				print "" > INFO_FILE

				#print " ----- LOAD_PER_DAY : " LOAD_PER_DAY
				OraclePartitions	= GetNoOfPartitions(is_partition) ;
				NoOfTablespaces     =  OraclePartitions ;
				NoOfIndexTablespaces     =  OraclePartitions ;

				print "----------------------------------------------------"> INFO_FILE
				printf ("Tablespace Info:\n") > INFO_FILE
				print "----------------------------------------------------"> INFO_FILE

			 	TotalTablespaceSize = GetTablespaceSize("TABLE",1);

				#printf("%-15s ==>  %s\n","OraclePartitions",OraclePartitions) > INFO_FILE
				#printf("%-15s ==>  %s\n","NoOfTablespaces",NoOfTablespaces) > INFO_FILE
				#printf("%-15s ==>  %s\n","NoOfIndexTablespaces",NoOfIndexTablespaces) > INFO_FILE
				#printf("%-15s ==>  %s\n","TotalTablespaceSize",TotalTablespaceSize) > INFO_FILE
			
				#print "OraclePartitions = "OraclePartitions ;
				#print "TotalTablespaceSize = " TotalTablespaceSize;

				PerTablespaceSize = GetTablespaceScript(NoOfTablespaces,TotalTablespaceSize,"TABLE") ; 
				NoOfTablesPerTSForNP = GetNoOfTablesPerTSForNP(TotalTablespaceSize) ;
				NoOfTSForNP	 = GetNoOfTSForNP(TotalTablespaceSize) ;
				#print "BEGIN ........ NoOfTablesPerTSForNP = " NoOfTablesPerTSForNP 
				#print "BEGIN ........ NoOfTSForNP = " NoOfTSForNP 
				GetTSNameForNPTable() ;
				GetTSNameForNPTableNew() ;
			}
			{ 
				xls_db_field_name = $2 ;
				xls_data_type = $3 ;
				xls_is_index = $4 ;
				DataType = DataTypeArray[xls_data_type] ;
				if (NR == NO_OF_DB_FIELDS)	
					EndChar = ""
				else
					EndChar = ","

				printf ("%-30s %s %s \n" , $2 , DataType,EndChar)   >DDLFile;
				
				if (xls_is_index ==1 )
				{
					IndexedFields=IndexedFields"|"xls_db_field_name
				}
			}
			END  \
			{

				NoOfIndexes = split (IndexedFields, IndexedFieldsArray , "|") ;
				NoOfIndexes = NoOfIndexes -1 ;
				#print "IndexedFields => " IndexedFields
				#print "NoOfIndexes = "NoOfIndexes ;
				TotalIndexTablespaceSize = GetTablespaceSize("INDEX",NoOfIndexes);
				GetTablespaceScript(NoOfTablespaces,TotalIndexTablespaceSize,"INDEX") ;
				GetTSNameForNPTableNew() ;
				for (i = 1 ; i <= NO_OF_PHYSICAL_TABLES ; i++)
				{
					TableName = MAIN_TABLE_NAME ;
					PrintTableStructure(TableName,is_partition,i) ;
					
					for (IndexNum = 2 ; IndexNum <= NoOfIndexes+1 ; IndexNum++)
					{
						IndexField = IndexedFieldsArray[IndexNum] ;
						IndexName = "IX_" RECORD_CONFIG_ID "_" i "_" substr(IndexField,1,2)"_" IndexNum ;
						PrintIndexStructure(MAIN_TABLE_NAME,is_partition,i,IndexNum,IndexName,IndexField) ;
					}
				}
				print "NoOfIndexTablespaces ====================> " NoOfIndexTablespaces > INFO_FILE
				print "NoOfTablespaces =========================> " NoOfTablespaces > INFO_FILE 
				print "----------------- End --------------------"
			}' $DATA_FILE

}
function RemoveTempFiles ()
{
	rm -f data_info.txt data_info.txt.tmp  excel_data.csv.tmp  header_info.txt  DDL.sql
}
function CopySQLFiles ()
{
	if [ ! -d $MAIN_TABLE_NAME ] 
	then
		mkdir -p $MAIN_TABLE_NAME
	fi
	mv $MAIN_TABLE_NAME"_DDL.sql" $MAIN_TABLE_NAME
#	mv "DROP_"$MAIN_TABLE_NAME"_INDEX_DDL.sql" $MAIN_TABLE_NAME
	mv "DROP_"$MAIN_TABLE_NAME"_DDL.sql" $MAIN_TABLE_NAME
	mv INFO.txt $MAIN_TABLE_NAME
	mv TS_TABLESPACE_LOCAL_$MAIN_TABLE_NAME.sql $MAIN_TABLE_NAME
	mv EXCEL_INPUT_"$MAIN_TABLE_NAME"_"$DateTime".csv $MAIN_TABLE_NAME
	mv TS_TABLESPACE_PROD_$MAIN_TABLE_NAME.sql $MAIN_TABLE_NAME
}
function main ()
{
	GEN_DIR=GENERATED_SQL_FILES
	TEMPLATE_DIR=SQL_TEMPLATES
	GenerateHeaderDataInfo
	GenerateTableStructure > $MAIN_TABLE_NAME"_DDL.sql"
	#RemoveTempFiles
	CopySQLFiles
}
main $*
