#! /bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
PREVEA_DBSETUP_HOME=/home/user/prevearoot/bin

GetTableNameInOrder ()
{
	ls $PREVEA_DBSETUP_HOME/*.sql >prevea_sqlfilelist.txt
	sqlFileList=`cat prevea_sqlfilelist.txt` 
	for i in $sqlFileList
	do
       		grep -i "CREATE TABLE" $i >> $WATIR_SERVER_HOME/Scripts/prevea_tables.txt
	done


	NoofTables=`cat $WATIR_SERVER_HOME/Scripts/prevea_tables.txt | wc -l`
    for (( i=1 ; i<=NoofTables ; i++ ))
    do
        Record=`cat $WATIR_SERVER_HOME/Scripts/prevea_tables.txt | head -$i | tail -1`
		TableName=`echo $Record | cut -d" " -f3`
		
		Tablelength=`echo $TableName | wc -c | cut -c1-8`
		Tablelength=`expr $Tablelength - 1`
		LastCharacterInTableName=`echo $TableName | cut -c $Tablelength`
		
		if [ $LastCharacterInTableName = "(" ]
		then 
			Tablelength=`expr $Tablelength - 1`
			TableName=`echo $TableName | cut -c 1-$Tablelength`
		fi	

		echo $TableName >> $WATIR_SERVER_HOME/Scripts/preveatablesinorder.txt 
	done	

        awk '{
                if ($0 in stored_lines)
                x=1
                else
                print
                stored_lines[$0]=1
        }' $WATIR_SERVER_HOME/Scripts/preveatablesinorder.txt  > $WATIR_SERVER_HOME/Scripts/UniquepreveaTablesInOrder.txt
}	

createATtable()
{
        echo "Creating AT table..."

        sqlplus -s prevea/prevea << EOF > /dev/null
        whenever oserror exit 1 ;
        SPOOL $WATIR_SERVER_HOME/Scripts/prevea_at_table;
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
        noOfTables=`cat $WATIR_SERVER_HOME/Scripts/UniquepreveaTablesInOrder.txt | wc -l`
        for (( i=1 ; i<=noOfTables; i++ ))
        do
                tableName=`cat $WATIR_SERVER_HOME/Scripts/UniquepreveaTablesInOrder.txt | head -$i | tail -1 | tr [:lower:] [:upper:]`

        sqlplus -s prevea/prevea << EOF > /dev/null
        whenever sqlerror exit 1 ;
        whenever oserror exit 1 ;
        SPOOL $WATIR_SERVER_HOME/Scripts/prevea_at_table_insert;
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
        rm -f $WATIR_SERVER_HOME/Scripts/prevea_tables.txt
        rm -f $WATIR_SERVER_HOME/Scripts/preveatablesinorder.txt
        rm -f $WATIR_SERVER_HOME/Scripts/prevea_dbce.sql

        GetTableNameInOrder
        createATtable
        insertTableNamesInATtable
		bash $WATIR_SERVER_HOME/Scripts/prevea_copyData_FromTables_ToTables.sh 2 BACKUPTABLE_
#        bash $WATIR_SERVER_HOME/Scripts/generateEntireSequenceCreationList.sh
}

main $*

