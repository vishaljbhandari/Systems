
$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;

delete from cdr ;
delete from subscr_neural_voice_profile ;
delete from subscr_neural_data_profile ;
--delete from subscriber_group_map where subscriber_id > 1024 ;
delete subscriber where id > 1024 ;
delete account where id > 1024 ;

update configurations set value = 2 where config_key = 'AI.NO_OF_CRG_VOICE_PROCESSORS' ;
update configurations set value = 5 where config_key = 'NumberOfPartitions' ;

insert into subscr_neural_voice_profile (subscriber_id, phone_number, network_id, sequence_id, profile_type, profile_matured)  values(1025, '+919820512340', 1, 0, 2, 1);
insert into subscr_neural_voice_profile (subscriber_id, phone_number, network_id, sequence_id, profile_type, profile_matured)  values(1026, '+919820512345', 1, 0, 2, 1);
insert into subscr_neural_voice_profile (subscriber_id, phone_number, network_id, sequence_id, profile_type, profile_matured)  values(1027, '+919820512342', 1, 0, 2, 1);
insert into subscr_neural_voice_profile (subscriber_id, phone_number, network_id, sequence_id, profile_type, profile_matured)  values(1028, '+919820512343', 1, 0, 2, 1);

commit;
quit ;
EOF
if test $? -eq 5
then
	exitval=1
else
	exitval=0
fi

exit $exitval

