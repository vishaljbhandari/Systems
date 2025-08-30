#! /bin/bash
. $RANGERHOME/sbin/rangerenv.sh
. $RUBY_UNIT_HOME/Scripts/configuration.sh

CurrentFileName="$(basename $0)"

GetTableNameInOrder ()
{
	ls $DBSETUP_HOME/*DDL*.sql >sqlfilelist.txt
	if [ $IS_RATING_ENABLED = "1" ]
		then 
		ls $RANGERV6_SERVER_HOME/Datasource/BalanceTrackingRater/*.sql* >> sqlfilelist.txt ## This is added for test rater to work
		ls $RANGERV6_SERVER_HOME/Datasource/BalanceTrackingRater/README >> sqlfilelist.txt ## this is added for test rater to work
	fi
	
	sqlFileList=`grep -vi "dashboard\|nikira-DDL-sparkdb\.sql" sqlfilelist.txt` ### here we exclude dashboard and sparkdb DDL files
	for i in $sqlFileList
	do
       		grep -i "CREATE TABLE" $i >> $RUBY_UNIT_HOME/Scripts/ranger_tables.txt
	done


	NoofTables=`cat $RUBY_UNIT_HOME/Scripts/ranger_tables.txt | wc -l`
    for (( i=1 ; i<=NoofTables ; i++ ))
    do
        Record=`cat $RUBY_UNIT_HOME/Scripts/ranger_tables.txt | head -$i | tail -1`
		TableName=`echo $Record | cut -d" " -f3`
		
		Tablelength=`echo $TableName | wc -c | cut -c1-8`
		Tablelength=`expr $Tablelength - 1`
		LastCharacterInTableName=`echo $TableName | cut -c $Tablelength`
		
		if [ $LastCharacterInTableName = "(" ]
		then 
			Tablelength=`expr $Tablelength - 1`
			TableName=`echo $TableName | cut -c 1-$Tablelength`
		fi	

		echo $TableName >> $RUBY_UNIT_HOME/Scripts/Rangertablesinorder.txt 
	done	

        awk '{
                if ($0 in stored_lines)
                x=1
                else
                print
                stored_lines[$0]=1
        }' $RUBY_UNIT_HOME/Scripts/Rangertablesinorder.txt  > $RUBY_UNIT_HOME/Scripts/UniqueRangerTablesInOrder.txt
}	

createATtable()
{
        echo "Creating AT table..."

        $DB_QUERY << EOF
        whenever oserror exit 1 ;
        SPOOL $RUBY_UNIT_HOME/Scripts/at_table;
        SET FEEDBACK ON ;
        SET SERVEROUTPUT ON ;
                drop table at_table;
                CREATE TABLE at_table (id NUMBER(3) NOT NULL,table_name VARCHAR2(30) NOT NULL);
        SPOOL OFF ;
    quit ;
EOF

return 0
}


insertTableNamesInATtable()
{
        noOfTables=`cat $RUBY_UNIT_HOME/Scripts/UniqueRangerTablesInOrder.txt | wc -l`
        for (( i=1 ; i<=noOfTables; i++ ))
        do
                tableName=`cat $RUBY_UNIT_HOME/Scripts/UniqueRangerTablesInOrder.txt | head -$i | tail -1 | tr [:lower:] [:upper:]`

        $DB_QUERY << EOF
	whenever sqlerror exit 1 ;
        whenever oserror exit 1 ;
        SPOOL $RUBY_UNIT_HOME/Scripts/at_table_insert;
        SET FEEDBACK ON ;
        SET SERVEROUTPUT ON ;
                INSERT INTO at_table VALUES ($i, '$tableName');
        commit;
        SPOOL OFF ;
    quit ;
EOF

        done

return 0
}


main ()
{
        rm -f $RUBY_UNIT_HOME/Scripts/ranger_tables.txt
        rm -f $RUBY_UNIT_HOME/Scripts/Rangertablesinorder.txt
        rm -f $RUBY_UNIT_HOME/Scripts/dbce.sql

        GetTableNameInOrder
        createATtable
        insertTableNamesInATtable
		bash $RUBY_UNIT_HOME/Scripts/copyData_FromTables_ToTables.sh 2 BACKUPTABLE_
        bash $RUBY_UNIT_HOME/Scripts/generateEntireSequenceCreationList.sh
		bash $RUBY_UNIT_HOME/Scripts/clearFlag.sh $CurrentFileName 
}

main $*

