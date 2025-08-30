$SQL_COMMAND -s /nolog << EOF > /dev/null
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
set serveroutput on ;
spool testipdrsummarywithoutpartitionV.dat;

select 1 from dual where 8 = (select count(*) from ipdr) ;
select 1 from dual where 1 = (select count(*) from ipdr_session_summary) ;
select 1 from dual where 1 = (select count(*) from ipdr_session_summary where phone_number = '+919843005761') ;
select 1 from dual where 1 = (select count(*) from ipdr_session_summary where duration = 300 ) ;

spool off ;
quit ;
EOF


SqlResult=$?
grep "no rows" testipdrsummarywithoutpartitionV.dat > /dev/null
if [ $? -eq  0 ] || [ $SqlResult -ne  0 ] ; then
	exitval=1
else
	exitval=0
fi
rm -f testipdrsummarywithoutpartitionV.dat
exit $exitval
