#! /bin/bash

. $RUBY_UNIT_HOME/Scripts/configuration.sh

getSQLfilesForDiffTables()
{
        for i in `cat $RUBY_UNIT_HOME/Scripts/Fixture/tmp/difftablelist.lst | grep "$PrefixForDiffTable"`
        do
                tableid=`echo $i | grep -o "[0-9]*$"`
		
		if [ $DB_TYPE == "1" ]
		then 
                table_name=`sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF 2> /dev/null | grep -v ^$
                                set heading off ;
                                set feedback off ;
                                        select table_name from AT_TABLE where id = $tableid;
                                quit ;
EOF`
		else
		table_name=`clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF 2> /dev/null | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved." | grep -v "set" | grep -v "select" | grep -v "quit" 
				set heading off ;
				set feedback off ;
					select table_name from AT_TABLE where id = $tableid;
				quit ;
EOF`
		fi
                spoolFileName="$RUBY_UNIT_HOME/Scripts/Fixture/tmp/$table_name.sql"
                bash $RUBY_UNIT_HOME/Scripts/createquery.sh $i $spoolFileName >/dev/null
        done

return 0
}

generateInsertQuery()
{
	cat $RUBY_UNIT_HOME/Scripts/Fixture/tmp/difftablelist.lst |grep -v ORA|grep -v DIFFTABLE|grep -v SQL > $RUBY_UNIT_HOME/Scripts/Fixture/tmp/TableNames.csv
	var=`cat $RUBY_UNIT_HOME/Scripts/Fixture/tmp/TableNames.csv`
	cd $RUBY_UNIT_HOME/Scripts/Fixture/tmp/
	echo "spool \$RUBY_UNIT_HOME/Scripts/Fixture/tmp/$FixtureName.log;" >$RUBY_UNIT_HOME/Scripts/Fixture/FixtureFiles/$FixtureName.sql
	for i in $var;do
    	sed "s/^/insert into $i values(/g" $i.sql.csv >temp.sql;
	    sed "s/$/);/g" temp.sql >withoutquotes.sql
    	sed "s/\`/\''/g" withoutquotes.sql>>$RUBY_UNIT_HOME/Scripts/Fixture/FixtureFiles/$FixtureName.sql
	done
	echo "commit;" >>$RUBY_UNIT_HOME/Scripts/Fixture/FixtureFiles/$FixtureName.sql
	echo "spool off;" >>$RUBY_UNIT_HOME/Scripts/Fixture/FixtureFiles/$FixtureName.sql
	echo "exit;" >>$RUBY_UNIT_HOME/Scripts/Fixture/FixtureFiles/$FixtureName.sql
return 0

}
getCommaSeparatedValues()
{
	for j in `ls $RUBY_UNIT_HOME/Scripts/Fixture/tmp/*.sql`
        do
                spoolFileName="$j.out"
if [ $DB_TYPE == "1" ]
		then
                sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF
                set serveroutput on;
                set heading off;
                set linesize 10000;
                set pagesize 1000;
				set trimspool on;
				set termout  off;
				set feedback off;
				set underline off;
				set echo off;
				set term off;
                spool $spoolFileName;
                        @$j;
                spool off;
/
EOF
	else
			clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
			set serveroutput on;
	                set heading off;
	                set linesize 10000;
	                set pagesize 1000;
					set trimspool on;
					set termout  off;
					set feedback off;
					set underline off;
					set echo off;
					set term off;
	                spool $spoolFileName;
	                        @$j;
	                spool off;
	/
EOF
	
	fi
	
	sed '/^$/d' $j.out>$j.temp
	sed 's/[ \t]*$//' $j.temp >inlinefile
	perl -pi -e "s/'/\`/g" inlinefile
	perl -pi -e "s/\|,\|/','/g" inlinefile
	perl -pi -e "s/^/'/g" inlinefile
	perl -pi -e "s/$/'/g" inlinefile
    perl -pi -e "s/^''/'/g" inlinefile
	sed '$d' inlinefile>$j.csv
	done

return 0
}

cleanup()
{
rm -f $RUBY_UNIT_HOME/Scripts/Fixture/tmp/*
}

main()
{	
	if [ $# -eq 3 ]
    then
    PrefixForBackUpTable=$1
	PrefixForDiffTable=$2
	FixtureName=$3
    else
        echo "Improper Usage: Three argument required 1:PrefixForBackUpTable 2:PrefixForDiffTable 3:FixtureName"
        exit 1
    fi
    cleanup
########## Drop existing DIFF tables
	bash $RUBY_UNIT_HOME/Scripts/droptables_ATtable.sh $PrefixForDiffTable >/dev/null

########## Create Diff tables and generate file difftablelist.lst
	bash $RUBY_UNIT_HOME/Scripts/createDiffTables.sh $PrefixForBackUpTable $PrefixForDiffTable  >/dev/null

########## for each DIFFTABLE fetch comma separated values
	getSQLfilesForDiffTables
	getCommaSeparatedValues
	generateInsertQuery
return 0
}

main $*
