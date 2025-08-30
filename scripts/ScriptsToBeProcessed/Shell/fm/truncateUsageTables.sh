#!/bin/bash

. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh


truncateTables()
{
if [ $USAGE_DB_TYPE == '1' ]
	then
	sqlplus -s $UsageServerDBUser/$UsageServerDBPassword@$UsageServerOracleSID<< EOF > /dev/null

	whenever sqlerror exit 1 ;
	whenever oserror exit 1 ;
	SPOOL $WATIR_SERVER_HOME/Scripts/tmp/truncateUsageTable_$UsageServerOracleSID.lst
	SET FEEDBACK ON ;
	SET SERVEROUTPUT ON ;
	declare
		cursor c1 is select distinct(tname) from tab where tname like '$CDR_TABLE_NAME_PREFIX%' OR tname like '$GPRS_TABLE_NAME_PREFIX%' OR tname like '$RECHARGELOG_TABLE_NAME_PREFIX%';
	begin
		for i in c1
		loop
		begin
			execute immediate 'TRUNCATE TABLE ' || i.TNAME;
	
		exception
			when others then
		    	dbms_output.put_line(sqlerrm) ;
	        end ;
		end loop;
		
		IF c1%ISOPEN
			THEN
			   CLOSE c1;
		END IF;

	end;
	/
	commit;
	SPOOL OFF ;
	quit ;
EOF
else
	
	vsql -U $UsageServerUser -p $UsageServerPort -h $UsageServerHost -w $UsageServerPassword -o $WATIR_SERVER_HOME/Scripts/TruncateUsgTablelist.txt -Atc "SELECT 'truncate table ' || TABLE_NAME || ';'FROM tables where TABLE_NAME like lower('$CDR_TABLE_NAME_PREFIX%') or TABLE_NAME like lower('$GPRS_TABLE_NAME_PREFIX%') or TABLE_NAME like lower('$RECHARGELOG_TABLE_NAME_PREFIX%') or TABLE_NAME like lower('$IPDR_TABLE_NAME_PREFIX%') or TABLE_NAME like lower('$LTE_TABLE_NAME_PREFIX%') or TABLE_NAME like lower('$ADJ_TABLE_NAME_PREFIX') or TABLE_NAME like lower('$CRM_TABLE_NAME_PREFIX');"
	
	vsql -U $UsageServerUser -p $UsageServerPort -h $UsageServerHost -w $UsageServerPassword -f	$WATIR_SERVER_HOME/Scripts/TruncateUsgTablelist.txt -o $WATIR_SERVER_HOME/Scripts/TruncateUsgTable.txt
	
	
fi	


return 0
}


main()
{
NoOfUsageServers=$((${#USAGEDB[@]}/3))
echo "NoOfUsageServers = $NoOfUsageServers"

if [ $NoOfUsageServers -gt 0 ]
	then
		for ((i=0;i<$NoOfUsageServers;i++));
		do
			if [ $USAGE_DB_TYPE == '1' ]
				then
					startIndex=`expr $i \* 3`
					UsageServerOracleSID=${USAGEDB[$startIndex]}
					UsageServerDBUser=${USAGEDB[$startIndex+1]}
					UsageServerDBPassword=${USAGEDB[$startIndex+2]}

					echo "TRUNCATING USAGE TABLES ON DB HAVING ORACLE_SID=$UsageServerOracleSID DBUSER=$UsageServerDBUser DBPASSWORD=$UsageServerDBPassword"
				
				else
					startIndex=`expr $i \* 4`
					UsageServerUser=${VERTICA_USAGEDB[$startIndex]}
					UsageServerPort=${VERTICA_USAGEDB[$startIndex+1]}
					UsageServerHost=${VERTICA_USAGEDB[$startIndex+2]}
					UsageServerPassword=${VERTICA_USAGEDB[$startIndex+3]}

					echo "TRUNCATING USAGE TABLES ON DB HAVING DBUSER=$UsageServerUser PORT=$UsageServerPort HOST=$UsageServerHost DBPASSWORD=$UsageServerPassword"
			fi
			truncateTables;
		done
	else
		echo "USAGEDB not set properly in configuration.sh !!!!!!!!"
		return 1
fi

return 0
}


main
