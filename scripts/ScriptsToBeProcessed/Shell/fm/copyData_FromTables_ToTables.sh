#! /bin/bash
. $RUBY_UNIT_HOME/Scripts/configuration.sh

copyFromDB2ExistingBackupTables()
{
        echo "copying data from original tables to Backup tables $PrefixForBackUpTable ...."

    $DB_QUERY << EOF > /dev/null
        whenever sqlerror exit 1 ;
        whenever oserror exit 1 ;
        SPOOL $RUBY_UNIT_HOME/Scripts/Fixture/tmp/createDBbackupToExisting;
        SET FEEDBACK ON ;
        SET SERVEROUTPUT ON ;
        declare
        cursor c1 is select * from at_table order by id;
        tname varchar2(50);
        begin
        for i in c1
        loop
                tname := '$PrefixForBackUpTable'||to_char(i.id);
        begin
		execute immediate 'TRUNCATE TABLE ' || tname;
                execute immediate 'insert into ' || tname || ' select * from ' || i.TABLE_NAME;
        exception
        when others then
                dbms_output.put_line(sqlerrm) ;
        end ;
end loop;
end;
/
        SPOOL OFF ;
    quit ;
EOF

return 0
}


copyFromBackup2DB()
{
	echo "copying data from Backup tables $PrefixForBackUpTable to original tables ...."

    $DB_QUERY << EOF > /dev/null
        whenever sqlerror exit 1 ;
        whenever oserror exit 1 ;
        SPOOL $RUBY_UNIT_HOME/Scripts/Fixture/tmp/createDBbackupToExistingDelete;
        SET FEEDBACK ON ;
        SET SERVEROUTPUT ON ;
        declare
        cursor c1 is select * from at_table order by id desc;
        begin
        for i in c1
        loop
        begin
                execute immediate 'delete from ' || i.TABLE_NAME;
		commit ;
        exception
        when others then
                dbms_output.put_line(sqlerrm) ;
        end ;
end loop;
commit;
end;
/
        SPOOL OFF ;
    quit ;
EOF

    $DB_QUERY << EOF > /dev/null
        whenever sqlerror exit 1 ;
        whenever oserror exit 1 ;
        SPOOL $RUBY_UNIT_HOME/Scripts/Fixture/tmp/createDBbackupToExistingInsert;
        SET FEEDBACK ON ;
        SET SERVEROUTPUT ON ;
        declare
        cursor c1 is select * from at_table order by id;
        tname varchar2(50);
        begin
        for i in c1
        loop
                tname := '$PrefixForBackUpTable'||to_char(i.id);
        begin
                execute immediate 'insert into ' || i.TABLE_NAME || ' select * from ' || tname;
                commit;
        exception
        when others then
                dbms_output.put_line(sqlerrm) ;
        end ;
end loop;
end;
/
        SPOOL OFF ;
    quit ;
EOF

return 0

}


createBackupTables()
{
	echo "creating backup tables with prefix $PrefixForBackUpTable ...."

    $DB_QUERY << EOF > /dev/null
        whenever sqlerror exit 1 ;
        whenever oserror exit 1 ;
        SPOOL $RUBY_UNIT_HOME/Scripts/Fixture/tmp/createDBbackup ;
        SET FEEDBACK ON ;
        SET SERVEROUTPUT ON ;
        declare
        cursor c1 is select * from at_table order by id;
        tname varchar2(50);
        begin
        for i in c1
        loop
                tname := '$PrefixForBackUpTable'||to_char(i.id);
        begin
                execute immediate 'create table ' || tname || ' as select * from ' || i.TABLE_NAME;
        exception
        when others then
                dbms_output.put_line(sqlerrm) ;
        end ;
end loop;
end;
/
        SPOOL OFF ;
    quit ;
EOF

return 0
}



checkIfbackUpTablesExist()
{
	backUpTablesExist=`$DB_QUERY << EOF 2> /dev/null | grep -v ^$
	set heading off ;
	set feedback off ;
		select count(*) from tab where tname like '$PrefixForBackUpTable%';	
	quit ;
	EOF`

return 0
}



main ()
{
    if [ $# -ne 2 ]
    then
	echo "Improper Usage: Two argument required "
	echo "First argument: 1/2    1:copy data from prefix table to original tables "
	echo "                       2:copy data to prefix table from original tables "
	echo "Second argument: PrefixForBackUpTable"
        exit 2
    else
        PrefixForBackUpTable=$2
	if [ $1 -eq 1 ]
	then
		copyFromBackup2DB
		touch $RUBY_UNIT_HOME/Scripts/Fixture/tmp/flagToRunProgrammanager

	elif [ $1 -eq 2 ]
	then
		checkIfbackUpTablesExist

		if [ $backUpTablesExist -eq 0 ]
		then
			createBackupTables
		else
			copyFromDB2ExistingBackupTables
		fi
	else
		echo "Improper Usage: First argument could only be 1/2 "
		echo "                1:Copy data from BackUp tables to original tables"
		echo "                2:Copy data from original tables to BackUp tables"
		exit 3
	fi

    fi

}

main $*
CurrentFileName="$(basename $0)"
bash $RUBY_UNIT_HOME/Scripts/clearFlag.sh $CurrentFileName
