#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

#*******************************************************************************
# *  Copyright (c) SubexAzure Limited 2006. All rights reserved.               *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Azure Limited. The program(s) may be used and/or copied with the written   *
# *  permission from SubexAzure Limited or in accordance with the terms and    *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
# *******************************************************************************
# Author - Sandeep Kumar Gupta

TABLE=$1
COUNT=$2
TIMEOUT=60

getTableCountOnSingleUsage()
{
if [ $USAGE_DB_TYPE == '1' ]
	then
	sqlplus -s $UsageServerDBUser/$UsageServerDBPassword@$UsageServerOracleSID<< EOF >/dev/null
    whenever sqlerror exit 1 ;
    whenever oserror exit 1 ;
    SPOOL $WATIR_SERVER_HOME/Scripts/tmp/tableCountOnUsageDB_$UsageServerOracleSID.lst
    SET FEEDBACK ON ;
    SET SERVEROUTPUT ON ;
    declare
        cursor c1 is select distinct(tname) from tab where tname like '$TABLE%';
		noOfTotalCount number;
		noOfSingleTableCount number;
    begin
		noOfTotalCount := 0;
        for i in c1
        loop
      begin
	  	noOfSingleTableCount :=0;
		dbms_output.put_line(i.TNAME) ;
        execute immediate 'SELECT COUNT(1) FROM ' || i.TNAME INTO noOfSingleTableCount;
		noOfTotalCount := noOfTotalCount + noOfSingleTableCount ;
      exception
          when others then
              dbms_output.put_line(sqlerrm) ;
          end ;
      end loop;
	  
	  IF c1%ISOPEN
	  THEN
	  		CLOSE c1;
	  END IF;
			  
	  	dbms_output.put_line('Record Count :'|| noOfTotalCount);
    end;
/
commit;
SPOOL OFF ;
quit ;
EOF
else
echo "inside else"	
	vsql -U $UsageServerUser -p $UsageServerPort -h $UsageServerHost -w $UsageServerPassword -o	$WATIR_SERVER_HOME/Scripts/WaitforTableCountlist.txt -Atc "SELECT 'SELECT COUNT(1) FROM ' || TABLE_NAME || ';' FROM	tables where TABLE_NAME like lower('$TABLE%');"
	
	vsql -U $UsageServerUser -p $UsageServerPort -h $UsageServerHost -w $UsageServerPassword -f	$WATIR_SERVER_HOME/Scripts/WaitforTableCountlist.txt -o $WATIR_SERVER_HOME/Scripts/Tablecounts.txt -At;
		
	
	noOfTotalCount=$(( $( cat $WATIR_SERVER_HOME/Scripts/Tablecounts.txt | tr "\n" "+" | xargs -I{} echo {} 0  ) ))
	# echo "Record Count :" $noOfTotalCount  > $WATIR_SERVER_HOME/Scripts/tmp/tableCountOnUsageDB_$UsageServerPort.lst
	# echo $noOfTotalCount  > $WATIR_SERVER_HOME/Scripts/tmp/RecordCount.txt
	# echo $TABLE  >> $WATIR_SERVER_HOME/Scripts/tmp/RecordCount.txt     

	
fi

return 0
}


getTotalTableCount()
{
NoOfUsageServers=$((${#USAGEDB[@]}/3))
echo "NoOfUsageServers = $NoOfUsageServers"
totalRecordCountOnAllUsage=0

if [ $NoOfUsageServers -gt 0 ]
    then
        for ((i=0;i<$NoOfUsageServers;i++));
        do
			recordCountOnSingleUsage=0
			if [ $USAGE_DB_TYPE == '1' ]
				then
					startIndex=`expr $i \* 3`
					UsageServerOracleSID=${USAGEDB[$startIndex]}
					UsageServerDBUser=${USAGEDB[$startIndex+1]}
					UsageServerDBPassword=${USAGEDB[$startIndex+2]}

					echo "GETTING TABLE COUNT ON DB HAVING ORACLE_SID=$UsageServerOracleSID DBUSER=$UsageServerDBUser DBPASSWORD=$UsageServerDBPassword"
					getTableCountOnSingleUsage;
					recordCountOnSingleUsage=`awk 'BEGIN { FS = ":" } /Record Count/ {print $2}' $WATIR_SERVER_HOME/Scripts/tmp/tableCountOnUsageDB_$UsageServerOracleSID.lst`
					totalRecordCountOnAllUsage=`expr $totalRecordCountOnAllUsage + $recordCountOnSingleUsage`
				else
					startIndex=`expr $i \* 4`
					UsageServerUser=${VERTICA_USAGEDB[$startIndex]}
					UsageServerPort=${VERTICA_USAGEDB[$startIndex+1]}
					UsageServerHost=${VERTICA_USAGEDB[$startIndex+2]}
					UsageServerPassword=${VERTICA_USAGEDB[$startIndex+3]}

					echo "GETTING TABLE COUNT ON DB HAVING DBUSER=$UsageServerUser PORT=$UsageServerPort HOST=$UsageServerHost DBPASSWORD=$UsageServerPassword"
					getTableCountOnSingleUsage;
					recordCountOnSingleUsage=`awk 'BEGIN { FS = ":" } /Record Count/ {print $2}' $WATIR_SERVER_HOME/Scripts/tmp/tableCountOnUsageDB_$UsageServerPort.lst`
					totalRecordCountOnAllUsage=`expr $totalRecordCountOnAllUsage + $recordCountOnSingleUsage`
			fi	
        done

    else
        echo "USAGEDB not set properly in configuration.sh !!!!!!!!"
        return 1
fi

return 0
}

updateTCscheduledTime()
{
############### here it is assumed that always in USAGEDB array 1st 3 entries will be for Spark Reference DB
    sqlplus -s ${SPARK_REFERENCE_DB[1]}/${SPARK_REFERENCE_DB[2]}@${SPARK_REFERENCE_DB[0]} << EOF
    update recurring_schedule set RSH_NEXT_DTTM=current_date + interval '1' second where RSH_ID in (select RSH_ID from FILE_COLLECTION);
    commit;
    quit ;
EOF
}

main()
{
	totalRecordCountOnAllUsage=0
	rm $WATIR_SERVER_HOME/Scripts/tmp/tableCountOnUsageDB_*

	nLoops=`expr $TIMEOUT / 2`
	j=0

	updateTCscheduledTime;

	while [ $j -lt $nLoops ] 
	do
		getTotalTableCount
			
		if [ $totalRecordCountOnAllUsage -ge $COUNT ] 
		then
			exitValue=0
			break;
		else
			exitValue=1
		fi

		sleep 0.2
		j=`expr $j + 1`
	done

	if [ $exitValue == '0' ]
	then
	    echo "Table Count Passed"
	else
	    echo "Wait for table count failed"
		exit 1
	fi

	 echo $totalRecordCountOnAllUsage  > $WATIR_SERVER_HOME/Scripts/tmp/RecordCount.txt
	 echo $TABLE  >> $WATIR_SERVER_HOME/Scripts/tmp/RecordCount.txt

					
}


main



