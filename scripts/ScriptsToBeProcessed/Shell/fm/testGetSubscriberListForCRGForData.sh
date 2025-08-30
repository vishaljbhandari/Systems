$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;
set serveroutput on ;

delete from subscr_neural_data_profile;
delete from subscr_neural_voice_profile;

insert into subscr_neural_data_profile (subscriber_id, phone_number, network_id, profile_type, sequence_id, profile_matured)
	values(4, '+918845200001', 1, 4, 0, 0) ;	

insert into subscr_neural_data_profile (subscriber_id, phone_number, network_id, profile_type, sequence_id, profile_matured)
	values(4, '+918845200002', 1024, 4, 0, 1) ;	

insert into subscr_neural_data_profile (subscriber_id, phone_number, network_id, profile_type, sequence_id, profile_matured)
	values(1025, '+919820535200',1,  4, 0, 1) ;	

insert into subscr_neural_data_profile (subscriber_id, phone_number, network_id, profile_type, sequence_id, profile_matured)
	values(1026, '+919820535201', 1, 5, 0, 1) ;	

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
	

