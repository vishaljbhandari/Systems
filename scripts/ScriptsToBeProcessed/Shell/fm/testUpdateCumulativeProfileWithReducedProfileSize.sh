

sqlplus $DB_USER/$DB_PASSWORD <<x
whenever oserror exit 5;
whenever sqlerror exit 5;

delete from subscr_neural_voice_profile ;
delete from subscr_neural_data_profile ;

insert into subscr_neural_voice_profile (subscriber_id, phone_number, sequence_id, profile_type, profile_matured, network_id)
values (1025, '+919820535200', 0, 2, 0, 1024) ;

insert into subscr_neural_voice_profile (subscriber_id, phone_number, sequence_id, profile_type, profile_matured, network_id)
values (1025, '+919820535200', 1, 2, 0, 1024) ;

insert into subscr_neural_data_profile (subscriber_id, phone_number, sequence_id, profile_type, profile_matured, network_id)
values (1025, '+919820535200', 0, 4, 1, 1024) ;

insert into subscr_neural_data_profile (subscriber_id, phone_number, sequence_id, profile_type, profile_matured, network_id)
values (1025, '+919820535200', 1, 4, 1, 1024) ;

commit;
quit;
x

if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

exit $exitval
~
~
~
