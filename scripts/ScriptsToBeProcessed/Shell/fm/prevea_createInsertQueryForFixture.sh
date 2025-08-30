#! /bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh

getSQLfilesForDiffTables()
{
        for i in `cat $WATIR_SERVER_HOME/Scripts/Fixture/tmp/difftablelist.lst | grep "$PrefixForDiffTable"`
        do
                tableid=`echo $i | grep -o "[0-9]*$"`

                table_name=`sqlplus -s prevea/prevea << EOF 2> /dev/null | grep -v ^$
                                set heading off ;
                                set feedback off ;
                                        select table_name from AT_TABLE where id = $tableid;
                                quit ;
                                EOF`

                spoolFileName="$WATIR_SERVER_HOME/Scripts/Fixture/tmp/$table_name.sql"
                bash $WATIR_SERVER_HOME/Scripts/prevea_createquery.sh $i $spoolFileName >/dev/null
        done

return 0
}

generateInsertQuery()
{
	cat $WATIR_SERVER_HOME/Scripts/Fixture/tmp/difftablelist.lst |grep -v ORA|grep -v DIFFTABLE > $WATIR_SERVER_HOME/Scripts/Fixture/tmp/TableNames.csv
	var=`cat $WATIR_SERVER_HOME/Scripts/Fixture/tmp/TableNames.csv`
	cd $WATIR_SERVER_HOME/Scripts/Fixture/tmp/
	echo "spool \$WATIR_SERVER_HOME/Scripts/Fixture/tmp/$FixtureName.log;" >$WATIR_SERVER_HOME/Scripts/Fixture/FixtureFiles/$FixtureName.sql
	for i in $var;do
    	sed "s/^/insert into $i values(/g" $i.sql.csv >temp.sql;
	    sed "s/$/);/g" temp.sql >withoutquotes.sql
    	sed "s/\`/\''/g" withoutquotes.sql>>$WATIR_SERVER_HOME/Scripts/Fixture/FixtureFiles/$FixtureName.sql
	done
	echo "commit;" >>$WATIR_SERVER_HOME/Scripts/Fixture/FixtureFiles/$FixtureName.sql
	echo "spool off;" >>$WATIR_SERVER_HOME/Scripts/Fixture/FixtureFiles/$FixtureName.sql
	echo "exit;" >>$WATIR_SERVER_HOME/Scripts/Fixture/FixtureFiles/$FixtureName.sql
return 0

}
getCommaSeparatedValues()
{
	for j in `ls $WATIR_SERVER_HOME/Scripts/Fixture/tmp/*.sql`
        do
                spoolFileName="$j.out"

                sqlplus -s prevea/prevea << EOF
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
rm -f $WATIR_SERVER_HOME/Scripts/Fixture/tmp/*
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
	bash $WATIR_SERVER_HOME/Scripts/prevea_droptables_ATtable.sh $PrefixForDiffTable >/dev/null

########## Create Diff tables and generate file difftablelist.lst
	bash $WATIR_SERVER_HOME/Scripts/prevea_createDiffTables.sh $PrefixForBackUpTable $PrefixForDiffTable  >/dev/null

########## for each DIFFTABLE fetch comma separated values
	getSQLfilesForDiffTables
	getCommaSeparatedValues
	generateInsertQuery
return 0
}

main $*
