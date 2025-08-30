$SQL_COMMAND /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

set heading off
spool testStoreCumulativeProfileV.dat

select 1 from dual where 1 = (select count(*) from subscriber_neural_profile where subscriber_id = 1025 
								and profile_type = 2 and profile_matured = 1 and network_id = 1024
								and	neural_profile1 = '0,1,1,0,25' and neural_profile2 = ',2.00,2,1;'
								and	neural_profile3 is null and neural_profile4 is null
								and maturity_info =  '5:10'
								and sequence_id = 0) ;

select 1 from dual where 1 = (select count(*) from subscriber_neural_profile where subscriber_id = 1025 
								and network_id = 1024 and profile_type = 2
								and	neural_profile1 is null and neural_profile2 is null
								and	neural_profile3 is null and neural_profile4 is null
								and sequence_id = 1) ;

select 1 from dual where 1 = (select count(*) from subscriber_neural_profile where subscriber_id = 1025 
								and profile_type = 4 and profile_matured = 0 and network_id = 1024
								and	neural_profile1 = '0,1,1,0,25' and neural_profile2 = ',2.00,2,1;'
								and	neural_profile3 is null and neural_profile4 is null
								and maturity_info =  '10:20'
								and sequence_id = 0) ;

select 1 from dual where 1 = (select count(*) from subscriber_neural_profile where subscriber_id = 1025 
								and network_id = 1024 and profile_type = 4
								and	neural_profile1 is null and neural_profile2 is null
								and	neural_profile3 is null and neural_profile4 is null
								and sequence_id = 1) ;

exit;
EOF

if ( [ $? -eq 5 ] || grep -i "no rows" testStoreCumulativeProfileV.dat )
then
	exitval=1
else
	exitval=0
fi
rm -f testStoreCumulativeProfileV.dat
exit $exitval
