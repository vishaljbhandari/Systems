#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh

tableName=$1
oracleSID=$2
dbUsername=$3
dbPassword=$4

truncTable()
{
	if [ $DB_TYPE == "1" ]
	then
	sqlplus -s $dbUsername/$dbPassword@$oracleSID << EOF > /dev/null
	whenever oserror exit 1 ;
	SPOOL $WATIR_SERVER_HOME/Scripts/tmp/truncateTable.lst;
	SET FEEDBACK ON ;
	SET SERVEROUTPUT ON ;
		truncate table $tableName;
		commit;
	SPOOL OFF ;
	quit ;
EOF
	else
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF > /dev/null
		whenever oserror exit 1 ;
		SPOOL $WATIR_SERVER_HOME/Scripts/tmp/truncateTable.lst;
		SET FEEDBACK ON ;
		SET SERVEROUTPUT ON ;
		truncate table $tableName;
		commit;
		SPOOL OFF ;
		quit ;
EOF
	
	fi
}


main()
{
	if [ $# -ne 4 ]
	then
		echo "Improper Usage: parameters required For Oracle :- 1:tableName 2:oracleSID 3:dbUsername 4:dbPassword"
		echo "					  For DB2    :- 1:tablename 2:NA	3:NA	     4: NA"
		echo "For DB2, make sure appropriate parameters are set in configurations file."
		exit 1
	else
		truncTable
	fi
}

main $*
