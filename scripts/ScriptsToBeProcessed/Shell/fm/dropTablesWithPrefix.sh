#! /bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh

droptables()
{
        echo "Dropping DB tables with prefix $TablePrefix ..."
if [ $DB_TYPE == "1" ]
then
    sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF
        whenever sqlerror exit 1 ;
        whenever oserror exit 1 ;
        SPOOL dropTableWithPrefix;
        SET FEEDBACK ON ;
        SET SERVEROUTPUT ON ;
        declare
        cursor c1 is select * from tab where tname like '$TablePrefix%';
        tname varchar2(50);
        begin
        for i in c1
        loop
        begin
               execute immediate 'drop table ' || i.tname;
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
else
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
        whenever sqlerror exit 1 ;
        whenever oserror exit 1 ;
        SPOOL dropTableWithPrefix;
        SET FEEDBACK ON ;
        SET SERVEROUTPUT ON ;
        declare
        cursor c1 is select TSCHEMA,TNAME,TABTYPE from tab where tname like '$TablePrefix%';
        tname varchar2(50);
        begin
        for i in c1
        loop
        begin
               execute immediate 'drop table ' || i.tname;
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

fi
}


main ()
{
    if [ $# -eq 1 ]
    then
        TablePrefix=$1
	droptables
    else
        echo "Improper Usage: one argument required 1:TablePrefix"
        exit 2
    fi
}

main $*

