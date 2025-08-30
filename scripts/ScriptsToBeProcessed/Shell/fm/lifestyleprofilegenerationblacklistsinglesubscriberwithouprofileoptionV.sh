FRAUD_SUBSCRIBER=1
FC_PHONE_NUMBER=2
FC_IMEI=3
FC_IMSI=4

$SQL_COMMAND /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool lifestyleprofilegenerationblacklistsinglesubscriberwithouprofileoptionV.dat

select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber where phone_number='+919845200005');
select 1 from dual where 1 = (select count(*) from account where id = 1025) ;
select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber where subscriber_type = $FRAUD_SUBSCRIBER); 

select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber 
									where phone_number='+919845200005'
										and subscriber_type = $FRAUD_SUBSCRIBER);

select 1 from dual where 0 = (select count(*) from usage_profiles
				where subscriber_id = 10253
					and profile_type = 'HOME'
					and vpmn = '111111'
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER); 

select 1 from dual where 0 = (select count(*) from usage_profiles
				where subscriber_id = 10253
					and vpmn = '111111'
					and profile_type = 'ROAMER'
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER); 

select 1 from dual where 0 = (select count(*) from usage_profiles
				where subscriber_id = 10253
					and vpmn = '111111'
					and profile_type = 'NRTRDE'
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER); 

select 1 from dual where 0 = (select count(*) from frq_called_destinations
				where subscriber_id = 10253
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and network_id = 1024
					and profile_type = 'HOME');

select 1 from dual where 0 = (select count(*) from frq_called_destinations
				where subscriber_id = 10253
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and network_id = 1024
					and profile_type = 'ROAMER');

select 1 from dual where 0 = (select count(*) from frq_called_destinations
				where subscriber_id = 10253
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and network_id = 1024
					and profile_type = 'NRTRDE');

select  1 from dual where 0 = (select count(*) from gprs_life_style_profiles
				where  phone_number = '+919845200005'
					and profile_type = 'HOME'
					and  subscriber_id  = 10253
					and  subscriber_type = $FRAUD_SUBSCRIBER);

select  1 from dual where 0 = (select count(*) from gprs_life_style_profiles
				where  phone_number = '+919845200005'
					and profile_type = 'ROAMER'
					and  subscriber_id  = 10253
					and  subscriber_type = $FRAUD_SUBSCRIBER);

select  1 from dual where 0 = (select count(*) from gprs_life_style_profiles
				where  phone_number = '+919845200005'
					and profile_type = 'NRTRDE'
					and  subscriber_id  = 10253
					and  subscriber_type = $FRAUD_SUBSCRIBER);

select  1 from dual where 0 = (select count(*) from subscriber_profile_dates where subscriber_id  = 10253 ) ;
select 1 from dual where 0 = (select count(*) from blacklist_profile_options ) ;
select 1 from dual where 0 = (select count(*) from suspect_values ) ;
spool off
EOF

if [ $? -eq '5' ] || grep "no rows" lifestyleprofilegenerationblacklistsinglesubscriberwithouprofileoptionV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f lifestyleprofilegenerationblacklistsinglesubscriberwithouprofileoptionV.dat
exit $exitval
