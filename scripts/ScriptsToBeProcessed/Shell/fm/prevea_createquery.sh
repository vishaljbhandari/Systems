#! /bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh

tableName=$1
spoolfilename=$2

echo "started to generate query for table $tableName..."

sqlplus -s prevea/prevea << EOF
set serveroutput on
set feedback off
spool $spoolfilename
DROP TYPE myarray;
create TYPE myarray as table of varchar2(200);
/
commit;
declare
        myquery1 varchar2(100);
        myquery2 varchar2(100);
        tname VARCHAR2(50) := '$tableName';
        v1 myarray;
        cursor tab_query is
        select decode(column_id,1,column_name,'||''|,|''||'||column_name) 
        from   user_tab_columns
        where  table_name = tname
        order by column_id;
       
BEGIN
        select 'select 'into myquery1 from sys.dual;
        open tab_query;
        loop
        fetch tab_query bulk collect into v1;
        exit when tab_query%NOTFOUND ;
        end loop;

        select 'from '||tname||' ;' into myquery2 from dual;

        dbms_output.put_line(myquery1);
        for i in 1 .. v1.count
        loop
         dbms_output.put_line(v1(i));
        end loop;
        dbms_output.put_line(myquery2);
END;
/
spool off;
EOF

echo "Done ..."
