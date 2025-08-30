$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL

whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off

spool blacklistrocprofileforbulkprocessing_verification.dat

select 1 from dual where 0 = (select count(*) from blacklist_profile_options where network_id = 1024) ;

select 1 from dual where 0 = (select count(*) from subscriber_neural_profile where subscriber_id=10251 and 
																					phone_number = '+919845000001') ;

select 1 from dual where 1 = (select count(*) from blacklist_neural_profile) ;

select 1 from dual where 1 = (select count(*) from blacklist_neural_profile where blacklist_id=10251 and 
																				  phone_number = '+919845000001') ;

select 1 from dual where 1 = (select count(*) from subscriber_neural_profile) ;


spool off
EOF

if [ $? -eq '5' ] || grep "no rows" blacklistrocprofileforbulkprocessing_verification.dat
then
	exitval=1
else
	exitval=0
fi
rm -f blacklistrocprofileforbulkprocessing_verification.dat
exit $exitval
