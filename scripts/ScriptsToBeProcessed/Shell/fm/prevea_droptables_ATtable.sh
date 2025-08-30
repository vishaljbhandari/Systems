#! /bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh

droptables()
{
        echo "Dropping DB tables with prefix $PrefixForBackUpTable ..."

    sqlplus -s prevea/prevea << EOF
        whenever sqlerror exit 1 ;
        whenever oserror exit 1 ;
        SPOOL createDBbackup ;
        SET FEEDBACK ON ;
        SET SERVEROUTPUT ON ;
        declare
        cursor c1 is select * from at_table ;
        tname varchar2(50);
        begin
        for i in c1
        loop
    		tname := '$PrefixForBackUpTable'||to_char(i.id);
        begin
               execute immediate 'drop table ' || tname;
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
}


main ()
{
    if [ $# -eq 1 ]
    then
        PrefixForBackUpTable=$1
	droptables
    else
        echo "Improper Usage: one argument required 1:PrefixForBackUpTable"
        exit 2
    fi
}

main $*

