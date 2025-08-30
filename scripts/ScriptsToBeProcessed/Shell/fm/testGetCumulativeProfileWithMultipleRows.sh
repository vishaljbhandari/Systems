
$SQL_COMMAND /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;

delete from subscr_neural_data_profile ;
delete from subscr_neural_voice_profile ;

-- Voice Profile

insert into subscr_neural_voice_profile (subscriber_id, phone_number, sequence_id, profile_type, profile_matured, network_id,  neural_profile1, neural_profile2, neural_profile3, neural_profile4, maturity_info) 
values (1025, '+919820535200', 0, 3, 1, 1024, '0,1,1,0,225,298.00,2,1;', '1,1,1,0,225,300.00,2,1;', '2,1,1,0,225,300.00,2,1;', '3,1,1,0,225,300.00,2,1;', '10:20') ;

insert into subscr_neural_voice_profile (subscriber_id, phone_number, sequence_id, profile_type, profile_matured, network_id,  neural_profile1, neural_profile2, neural_profile3, neural_profile4, maturity_info) 
values (1025, '+919820535200', 1, 3, 1, 1024, '4,1,1,0,225,298.00,2,1;', '5,1,1,0,225,300.00,2,1;', '6,1,1,0,225,300.00,2,1;', '7,1,1,0,225,300.00,2,1;', '') ;

insert into subscr_neural_voice_profile (subscriber_id, phone_number, sequence_id, profile_type, profile_matured, network_id,  neural_profile1, neural_profile2, neural_profile3, neural_profile4, maturity_info) 
values (1026, '+919820535201', 0, 3, 1, 1024, '0,1,1,0,235,298.00,2,1;', '1,1,1,0,225,300.00,2,1;', '2,1,1,0,225,300.00,2,1;', '3,1,1,0,225,300.00,2,1;', '20:40') ;

---- Data Profile

insert into subscr_neural_data_profile (subscriber_id, phone_number, sequence_id, profile_type, profile_matured, network_id,  neural_profile1, neural_profile2, neural_profile3, neural_profile4, maturity_info) 
values (1025, '+919820535200', 0, 5, 1, 1024, '0,1,1,0,325,298.00,2,1;', '1,1,1,0,225,300.00,2,1;', '2,1,1,0,225,300.00,2,1;', '3,1,1,0,225,300.00,2,1;', '5:10') ;

insert into subscr_neural_data_profile (subscriber_id, phone_number, sequence_id, profile_type, profile_matured, network_id,  neural_profile1, neural_profile2, neural_profile3, neural_profile4, maturity_info) 
values (1025, '+919820535200', 1, 5, 1, 1024, '4,1,1,0,325,298.00,2,1;', '5,1,1,0,225,300.00,2,1;', '6,1,1,0,225,300.00,2,1;', '7,1,1,0,225,300.00,2,1;', '') ;

insert into subscr_neural_data_profile (subscriber_id, phone_number, sequence_id, profile_type, profile_matured, network_id,  neural_profile1, neural_profile2, neural_profile3, neural_profile4, maturity_info) 
values (1026, '+919820535201', 0, 5, 1, 1024, '0,1,1,0,335,298.00,2,1;', '1,1,1,0,225,300.00,2,1;', '2,1,1,0,225,300.00,2,1;', '3,1,1,0,225,300.00,2,1;', '10:20') ;

commit;
quit;
EOF

if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

exit $exitval
