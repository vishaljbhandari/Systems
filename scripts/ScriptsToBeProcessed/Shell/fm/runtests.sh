#!/bin/bash

sqlplus -s -l $DB_USER/$DB_PASSWORD > /dev/null <<END 
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
spool testforutplsql.dat ;
select 1 from user_objects where OBJECT_NAME='UTPLSQL' ;
spool off ;
quit ;
END
sql_ret="$?"
grep "no rows selected" testforutplsql.dat > /dev/null
if [ $? -eq 0 -o "$sql_ret""5" = "5""5" ]; then
	echo "Run <DBSetup Path>/Tests/utplsqlsetup.sh and then run the tests"
	rm -f testforutplsql.dat
	exit 1
fi
rm -f testforutplsql.dat

TEST_DIR=`dirname $0`
cd $TEST_DIR
sqlplus -s -l $DB_USER/$DB_PASSWORD <<EOF | grep -v '^>'
set feedback off
--Configuration---
set serveroutput on size 100000 ;
exec utconfig.setPrefix ('test_') ;
exec utConfig.setdir('$PWD') ;
exec utConfig.showfailuresonly(true) ;

--Add Tests Here---

exec utplsql.test('generatehighcostsummary') ;
exec utplsql.test('switchtrunksummary') ;

EOF
