#!/bin/bash

. $RUBY_UNIT_HOME/Scripts/configuration.sh

PrefixForBackUpTable=$1
PrefixForDiffTable=$2

echo "creating Diff tables ..."

if [ $DB_TYPE == "1" ]
then 

sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF
set heading off ;
set feedback off ;
set serveroutput on;
spool $RUBY_UNIT_HOME/Scripts/Fixture/tmp/difftablelist.lst;
declare
	rec_count number;
	table1 varchar2(50);
	table2 varchar2(50);
	table3 varchar2(50);
	v_table varchar2(1000);
	diffCountQry varchar(2048);
	cursor c1 is select * from at_table where table_name not in ('RANGER_SESSIONS','AUDIT_TRAILS','PM_RUNS','PM_JOURNAL','FMS_THREAD_JOURNAL') order by id;
BEGIN
	for i in c1
        loop
		table1 := to_char(i.table_name);
		table2 := '$PrefixForBackUpTable'||to_char(i.id);
		table3 := '$PrefixForDiffTable'||to_char(i.id);
		rec_count := 0;
		v_table := '(select * from ' || table1 || ' MINUS select * from ' || table2 || ')' ;
        begin
		execute immediate 'select count(*) from ' ||v_table INTO rec_count ;
		if rec_count > 0
		then
		BEGIN
			execute immediate 'create table '||table3||' as select * from ( select * from '||table1||' MINUS select * from '||table2||')';
			dbms_output.put_line(table1) ;
			dbms_output.put_line(table3) ;
		END;
		end if ;
        	exception
	        when others then
        	        dbms_output.put_line(sqlerrm) ;
        end ;
	end loop;

end;
/
spool off;
EOF

else

clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME <<EOF
set heading off ;
set feedback off ;
set serveroutput on;
spool $RUBY_UNIT_HOME/Scripts/Fixture/tmp/difftablelist.lst;
declare
	rec_count number;
	table1 varchar2(50);
	table2 varchar2(50);
	table3 varchar2(50);
	v_table varchar2(1000);
	diffCountQry varchar(2048);
	cursor c1 is select ID,TABLE_NAME from at_table where table_name not in ('RANGER_SESSIONS','AUDIT_TRAILS','PM_RUNS','PM_JOURNAL','FMS_THREAD_JOURNAL') order by id;
BEGIN
	for i in c1
        loop
		table1 := to_char(i.table_name);
		table2 := '$PrefixForBackUpTable'||to_char(i.id);
		table3 := '$PrefixForDiffTable'||to_char(i.id);
		rec_count := 0;
		v_table := '(select * from ' || table1 || ' MINUS select * from ' || table2 || ')' ;
        begin
		execute immediate 'select count(*) from ' ||v_table INTO rec_count ;
		if rec_count > 0
		then
		BEGIN
			execute immediate 'create table '||table3||' as select * from ( select * from '||table1||' MINUS select * from '||table2||')';
			dbms_output.put_line(table1) ;
			dbms_output.put_line(table3) ;
		END;
		end if ;
        	exception
	        when others then
        	        dbms_output.put_line(sqlerrm) ;
        end ;
	end loop;

end;
/
spool off;
EOF

fi

