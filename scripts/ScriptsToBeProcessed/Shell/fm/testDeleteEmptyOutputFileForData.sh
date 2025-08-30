
rm  $COMMON_MOUNT_POINT/FMSData/AICumulativeGPRSData/Process/*
rm  $COMMON_MOUNT_POINT/FMSData/AICumulativeGPRSData/Process/success/*

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;

delete from gprs_cdr ;
delete from subscr_neural_data_profile ;
delete from subscr_neural_voice_profile ;
--delete from subscriber_group_map where subscriber_id > 1024 ;
delete subscriber where id > 1024 ;
delete account where id > 1024 ;

insert into subscr_neural_data_profile (subscriber_id, phone_number, network_id, sequence_id, profile_type, profile_matured)  values(1025, '+919820535200', 1, 0, 4, 1);
insert into subscr_neural_data_profile (subscriber_id, phone_number, network_id, sequence_id, profile_type, profile_matured)  values(1026, '+919820535201', 1, 0, 4, 1);

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

