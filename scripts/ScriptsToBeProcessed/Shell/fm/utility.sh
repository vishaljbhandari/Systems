#! /usr/bin/env bash

dateTimeDataType=4
recordDirectory="$COMMON_MOUNT_POINT/FMSData/RecordLoaderData"
successDirectory="$recordDirectory/success"
prevkey=""
mainlog="$recordDirectory/log/recordloader.log"
exchangelog="$recordDirectory/log/partitionexchange.log"
syncexchangelog="$recordDirectory/log/syncpartitionexchange.log"
errorlog="$recordDirectory/log/errors.log"
tempsqloutput="$recordDirectory/log/.tempsqloutput"

TEMPDIR="$COMMON_MOUNT_POINT/tmp"
parameterFile="$RANGERHOME/bin/parameter.file"
ReceivedExitCommand=0

>$tempsqloutput

recordType=0
tableName=''
delimeter=','
sleepTime=2
minRecords=500000
bExit=0
tempRecordCount=0
syncByPartitionExchangeWaitTime=30
syncByPartitionExchangeMaxAttemptCount=20
syncByDirectInsertionWaitTime=30
syncByDirectInsertionMaxAttemptCount=20
MaxAttemptCount=20
MaxWaitTime=30

# To Get the max wait time from configurations table
getMaxWaitTime()
{

	AttemptCount=0 ;
    while true
    do
       $SQL_COMMAND -s /NOLOG << EOF > $tempsqloutput  2>&1
	CONNECT_TO_SQL
	WHENEVER OSERROR  EXIT 6 ;
	WHENEVER SQLERROR EXIT 5 ;
	SPOOL $TEMPDIR/maxWaitTimeForRecType.lst
	        SELECT MAX_WAIT_TIME FROM DBWRITER_CONFIGURATIONS WHERE RECORD_CONFIG_ID=$recordType ;
		COMMIT ;
EOF
	sqlretcode=`echo $?`
           if [ $sqlretcode -ne 0 ]
               then
                   AttemptCount=`expr $AttemptCount + 1 `
                   if [ $AttemptCount -gt $MaxAttemptCount ]
                       then
                           echo "---------------------------------------------------\n`date` \nCouldn't Get Max Wait Time For Record Type $recordType "  >> $errorlog
                           sqlErrorCheck $sqlretcode
                           fi
                           sleep $MaxWaitTime ;
       continue
                   else
                       break ;
       fi
           done
	sqlretcode=`echo $?`
	cat $tempsqloutput | grep "no rows selected" > /dev/null 2>&1
	norows=`echo $?`
	sqlErrorCheck $sqlretcode

	if [ $norows -eq 0 ]
	then
		maxWaitTimeArray[$recordType]=1
	else
		dbCount=`grep "[0-9][0-9]*" $TEMPDIR/maxWaitTimeForRecType.lst`
		maxWaitTimeArray[$recordType]=`echo $dbCount | tr -d " "`

	fi

	rm $TEMPDIR/maxWaitTimeForRecType.lst
}

# To Get the min Records per recordType
getMinRecordsForRecordType()
{
	AttemptCount=0 ;
    while true
    do
       $SQL_COMMAND -s /NOLOG << EOF > $tempsqloutput  2>&1
	CONNECT_TO_SQL
	WHENEVER OSERROR  EXIT 6 ;
	WHENEVER SQLERROR EXIT 5 ;
	SPOOL $TEMPDIR/minRecordsForRecorType.lst
		SELECT MAX_RECS_PER_SUBPARTITION FROM DBWRITER_CONFIGURATIONS WHERE RECORD_CONFIG_ID=$recordType ;
		COMMIT ;
EOF
	sqlretcode=`echo $?`
           if [ $sqlretcode -ne 0 ]
               then
                   AttemptCount=`expr $AttemptCount + 1 `
                   if [ $AttemptCount -gt $MaxAttemptCount ]
                       then
                           echo "---------------------------------------------------\n`date` \nCouldn't Get Max Recs Per Subpartition For Record Type $recordType "  >> $errorlog
                           sqlErrorCheck $sqlretcode
                           fi
                           sleep $MaxWaitTime ;
       continue
                   else
                       break ;
       fi
           done
	cat $tempsqloutput | grep "no rows selected" > /dev/null 2>&1
	norows=`echo $?`
	sqlErrorCheck $sqlretcode

	if [ $norows -eq 0 ]
	then
		minRecordsArray[$recordType]=500000
	else
		dbCount=`grep "[0-9][0-9]*" $TEMPDIR/minRecordsForRecorType.lst`
		minRecordsArray[$recordType]=`echo $dbCount | tr -d " "`
	fi

	rm $TEMPDIR/minRecordsForRecorType.lst
}

# Truncates TEMP_Record_$ table
truncateTempTable()
{
	tempTableName=$1

	AttemptCount=0 ;
    while true
    do
	$SQL_COMMAND -s /nolog << EOF > $tempsqloutput  2>&1
	CONNECT_TO_SQL
	WHENEVER OSERROR  EXIT 6 ;
	WHENEVER SQLERROR EXIT 5 ;
		TRUNCATE TABLE $tempTableName ;
	QUIT
EOF
	sqlretcode=`echo $?`
           if [ $sqlretcode -ne 0 ]
               then
                   AttemptCount=`expr $AttemptCount + 1 `
                   if [ $AttemptCount -gt $MaxAttemptCount ]
                       then
                           echo "---------------------------------------------------\n`date` \nCouldn't Truncate table $tempTableName "  >> $errorlog
                           sqlErrorCheck $sqlretcode
                           fi
                           sleep $MaxWaitTime ;
       continue
                   else
                       break ;
       fi
           done
	sqlErrorCheck $sqlretcode
}

# On SIGTERM or SIGINT provide a safe EXIT
exitCheck()
{
    if [ $ReceivedExitCommand -eq 1 ]
    then
		echo "SIGTERM Recieved: Initiated termination..."
		echo "Patience ... "

		echo "SIGTERM Recieved: Successfully terminated" >> $mainlog
		echo "RecordLoader End Time: `date`" >> $mainlog
		exit 0

		for table_name in ${tableName[@]}
		do
			rm -f $TEMPDIR/$table_name.ctl.TEMPLATE ;
		done
    fi
	test -f $tempsqloutput
	if [ "$?" -eq 0 ]
    then
			rm $tempsqloutput
    fi
}

# Trap SIGTERM and SIGINT signals
on_exit ()
{
		for table_name in ${tableName[@]}
		do
			rm -f $TEMPDIR/$table_name.ctl.TEMPLATE ;
		done
	test -f $tempsqloutput
	if [ "$?" -eq 0 ]
    then
			rm $tempsqloutput
    fi
}

on_sigterm()
{
	echo "received sigterm ... `date`" >> $mainlog
	ReceivedExitCommand=1
}

on_sighup()
{
	echo "SIGHUP Recieved." >> $mainlog
}

getColumns()
{
	recordType=$1
	ctlFileName="$TEMPDIR/${tableName[$recordType]}".ctl 

	if [ ! -f $ctlFileName.TEMPLATE ]
	then
	AttemptCount=0 ;
   while true
   do

		$SQL_COMMAND -s /nolog << EOF > $tempsqloutput  2>&1
		CONNECT_TO_SQL
		WHENEVER OSERROR  EXIT 6 ;
		WHENEVER SQLERROR EXIT 5 ;
			SET LINES 500 HEAD OFF PAGESIZE 0 FEEDBACK OFF COLSEP '|' ;
			SPOOL $TEMPDIR/columns.lst
			SELECT NVL(database_field,''), NVL(position,0) position, field_id, data_type
				FROM field_configs
				WHERE record_config_id = $recordType 
					and database_field is not null
					ORDER BY position, field_id ;
			SPOOL OFF ;
EOF
			sqlretcode=`echo $?`
           if [ $sqlretcode -ne 0 ]
               then
                   AttemptCount=`expr $AttemptCount + 1 `
                   if [ $AttemptCount -gt $MaxAttemptCount ]
                       then
                           echo "---------------------------------------------------\n`date` \nCouldn't Truncate table $tempTableName "  >> $errorlog
                           sqlErrorCheck $sqlretcode
                           fi
                           sleep $MaxWaitTime ;
       continue
                   else
                       break ;
       fi
           done
			cat $tempsqloutput | grep "no rows selected" > /dev/null 2>&1
			if [ $sqlretcode -eq 0 -a "$?" -eq 0 ]
			then
				rm $TEMPDIR/columns.lst
				errorMessage="Statement failed:\n Unable to get Database fields for $recordType"
				logErrorMessage "$errorMessage"
			fi
			sqlErrorCheck $sqlretcode
			noofColumns=`cat $TEMPDIR/columns.lst | wc -l| tr -d " "`
			generateControlFile	
			rm $TEMPDIR/columns.lst
	fi
		ChangePartitionParams ;
}
ChangePartitionParams()
{
	sed -e "s/#DAY_OF_YEAR#/\"${partitionKeyArray[$recordType]}\"/" -e "s/#HOUR_OF_DAY#/\"${subpartitionKeyArray[$recordType]}\"/" -e "s/#CTL_KEY#/$ctl_key/" $ctlFileName.TEMPLATE >$ctlFileName ;
}

generateControlFile()
{
	ctlFileNameTemplate="$TEMPDIR/${tableName[$recordType]}".ctl.TEMPLATE

	echo -e "\nLOAD DATA \c" > $ctlFileNameTemplate 
	echo -e "\nAPPEND\c" >> $ctlFileNameTemplate 
	tmpTableName=TEMP_"${tableName[$recordType]}"_"#CTL_KEY#" 
	echo -e "\nINTO TABLE "$tmpTableName >> $ctlFileNameTemplate
	echo -e "FIELDS TERMINATED BY '"$delimeter"'" >> $ctlFileNameTemplate  
	echo -e "TRAILING NULLCOLS \c" >> $ctlFileNameTemplate 
	echo -e "\n(\c" >> $ctlFileNameTemplate 
	dayOfYear="DAY_OF_YEAR"	
	hourOfDay="HOUR_OF_DAY"	


	for (( i=1; i <= $noofColumns; ++i)) 
	do
		column_name=`head -$i $TEMPDIR/columns.lst | tail -1 | tr -d " " | cut -d "|" -f1`	
		column_data_type=`head -$i $TEMPDIR/columns.lst | tail -1 | tr -d " " | cut -d "|" -f4`	
	
		if [ $column_data_type -eq 4 ] 
		then 
			echo -e "\n\t$column_name \"to_date(:$column_name, 'yyyy/mm/dd hh24:mi:ss')\" \c" >> $ctlFileNameTemplate
		else
			echo -e "\n\t$column_name\c" >> $ctlFileNameTemplate		
			if [ $column_name == $dayOfYear ] 
				then
					echo -e " #$dayOfYear#\c" >> $ctlFileNameTemplate 
				elif [ $column_name == $hourOfDay ]
				then
					echo -e " #$hourOfDay#\c" >> $ctlFileNameTemplate 
			fi ;
		
		fi ;

		if [ "$i" != "$noofColumns" ] 
		then 
			echo -e ",\c" >> $ctlFileNameTemplate	
		fi ;
	done	
	echo -e "\n)" >> $ctlFileNameTemplate	
}

# Index Rebuild
rebuildTempRecordIndexes()
{
	tempTableName=$1

	AttemptCount=0 ;
   while true
   do
       $SQL_COMMAND -s /NOLOG << EOF > $tempsqloutput 2>&1
	CONNECT_TO_SQL
	WHENEVER OSERROR  EXIT 6 ;
	WHENEVER SQLERROR EXIT 5 ;
	SET SERVEROUTPUT OFF
        SET FEEDBACK OFF
        SET HEADING OFF
        SPOOL $TEMPDIR/indexcount.lst
		SELECT INDEX_NAME FROM USER_INDEXES WHERE TABLE_NAME = UPPER('$tempTableName') ;
        SPOOL OFF ;
EOF
	sqlretcode=`echo $?`
           if [ $sqlretcode -ne 0 ]
               then
                   AttemptCount=`expr $AttemptCount + 1 `
                   if [ $AttemptCount -gt $MaxAttemptCount ]
                       then
                           echo "---------------------------------------------------\n`date` \nCouldn't Truncate table $tempTableName "  >> $errorlog
                           sqlErrorCheck $sqlretcode
                           fi
                           sleep $MaxWaitTime ;
       continue
                   else
                       break ;
       fi
           done
	sqlErrorCheck $sqlretcode

        >$TEMPDIR/index_names.txt
        for word in `cat $TEMPDIR/indexcount.lst`
        do
        	echo $word >> $TEMPDIR/index_names.txt 2>&1
        done

        for indexname in `cat $TEMPDIR/index_names.txt`
        do
		$SQL_COMMAND -s /nolog << EOF > $tempsqloutput  2>&1
		CONNECT_TO_SQL
		WHENEVER OSERROR  EXIT 6 ;
		WHENEVER SQLERROR EXIT 5 ;
			ALTER INDEX $indexname NOLOGGING PARALLEL ;
			ALTER INDEX $indexname REBUILD ;
			ALTER INDEX $indexname LOGGING NOPARALLEL ;
EOF
		sqlretcode=`echo $?`	
		sqlErrorCheck $sqlretcode
        done
	
        rm $TEMPDIR/indexcount.lst
        rm $TEMPDIR/index_names.txt
}

startScript(){
	
	command="$1"
	name=$2

	PIDFileName="$TEMPDIR/$2.pid"

	checkRunScript

	$command $PIDFileName
	return $?
}

checkRunScript(){

	if [ -f $PIDFileName ]
	then
		pidVal=`grep "[0-9][0-9]*" $PIDFileName`
		pscommand="ps -u $LOGNAME"
		$pscommand | grep $pidVal | grep $name | grep -v grep > test
		if [ "$?" -eq 0 ]
		then
			rm test
			sleep $sleepTime
			checkRunScript
		fi
		rm test
	fi
	
}
