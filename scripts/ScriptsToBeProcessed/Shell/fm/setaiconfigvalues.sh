
$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
set serveroutput on ;

delete from ani_prefix ;

insert into ani_prefix (prefix, description) values('+91', '') ;
insert into ani_prefix (prefix, description) values('+92', '') ;

delete from ai_groups ;
insert into ai_groups values('Default');
insert into ai_groups values('Subscriberless');

delete from configurations where config_key = 'AI.AVG_VOICE_CDR_COUNT_THRESHOLD';
insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'AI.AVG_VOICE_CDR_COUNT_THRESHOLD', 0) ;

delete from configurations where config_key = 'AI.PROFILE_GEN_THRESHOLD_VOICE' ;
insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'AI.PROFILE_GEN_THRESHOLD_VOICE', 10) ;

delete from configurations where config_key = 'AI.AVG_DATA_CDR_COUNT_THRESHOLD';
insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'AI.AVG_DATA_CDR_COUNT_THRESHOLD', 0) ;

delete from configurations where config_key = 'AI.PROFILE_GEN_THRESHOLD_DATA' ;
insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'AI.PROFILE_GEN_THRESHOLD_DATA', 10) ;

--delete from subscriber_group_map where subscriber_id = 4 ;
--insert into subscriber_group_map values('Subscriberless', 4) ;

delete from configurations where config_key = 'AI.CUMREC_VOICE_LAST_PROCESSED_DAY_OF_YEAR_0' ;
insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'AI.CUMREC_VOICE_LAST_PROCESSED_DAY_OF_YEAR_0', '1') ;

delete from configurations where config_key = 'AI.CUMREC_DATA_LAST_PROCESSED_DAY_OF_YEAR_0' ;
insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'AI.CUMREC_DATA_LAST_PROCESSED_DAY_OF_YEAR_0', '1') ;

update configurations set value = 1 where config_key = 'AI.NO_OF_VOICE_NPG_PROCESSORS' ;
update configurations set value = 1 where config_key = 'AI.NO_OF_DATA_NPG_PROCESSORS' ;

update configurations set value = 0 where config_key = 'AI.MAX_SUBSCRIBER_PER_FETCH' ;

update configurations set value = 0 where config_key like 'AI.LAST_VOICE_SUB_ID%' ;
update configurations set value = 0 where config_key like 'AI.LAST_DATA_SUB_ID%' ;

update configurations set value = 1 where config_key = 'AI.ENABLE_CUMULATIVE_VOICE_PROFILE' ;
update configurations set value = 1 where config_key = 'AI.ENABLE_CUMULATIVE_DATA_PROFILE' ;

update configurations set value = 1 where config_key = 'AI.NO_OF_CRG_VOICE_PROCESSORS' ;
update configurations set value = 1 where config_key = 'AI.NO_OF_CRG_DATA_PROCESSORS' ;

update configurations set value = 4 where config_key = 'AI.CUMREC_VOICE_LOOKBACK_DAYS' ;
update configurations set value = 4 where config_key = 'AI.CUMREC_DATA_LOOKBACK_DAYS' ;
update configurations set value = 364 where config_key = 'CDR Cleanup Interval In Days' ;

update configurations set value = '' where config_key like 'LAST_CDR_TIME%' ;
update configurations set value = '' where config_key like 'LAST_GPRS_CDR_TIME%' ;
update configurations set value = 1  where config_key = 'NumberOfPartitions' ;
update configurations set value = 1  where config_key = 'GPRSNumberOfPartitions' ;

update configurations set value = '10000' where config_key = 'AI.MAX_SUBSCRIBER_PER_AICRG_FILE' ;

commit ;
quit ;
EOF

if test $? -eq 5 
then
    exitval=1
else
    exitval=0
fi

echo $exitval
exit $exitval
