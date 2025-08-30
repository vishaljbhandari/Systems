FRAUD_SUBSCRIBER=1
FC_PHONE_NUMBER=2
FC_IMEI=3
FC_IMSI=4

$SQL_COMMAND /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool lifestyleprofilegenerationblacklistsinglesubscriberV.dat

select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber where phone_number='+919845200005');
select 1 from dual where 1 = (select count(*) from account where id = 1025) ;
select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber where subscriber_type = $FRAUD_SUBSCRIBER); 

select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber 
									where phone_number='+919845200005'
										and subscriber_type = $FRAUD_SUBSCRIBER);

select 1 from dual where 1 = (select count(*) from usage_profiles
				where record_type = 1
					and call_type = 1
					and subscriber_id = 10253
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and profile_type = 'HOME' 
					and vpmn = '111111'
					and cdr_count = 2
					and avg_duration = 40                             
					and avg_value = 10); 

select 1 from dual where 1 = (select count(*) from usage_profiles
				where record_type = 1
					and call_type = 1
					and subscriber_id = 10253
					and phone_number = '+919845200005'                             
					and vpmn = '111111'
					and subscriber_type = $FRAUD_SUBSCRIBER
					and profile_type = 'ROAMER' 
					and cdr_count = 1
					and avg_duration = 40                             
					and avg_value = 10); 

select 1 from dual where 1 = (select count(*) from usage_profiles
				where record_type = 1
					and call_type = 1
					and subscriber_id = 10253
					and vpmn = '111111'
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and profile_type = 'NRTRDE' 
					and cdr_count = 1
					and avg_duration = 40                             
					and avg_value = 10); 

select 1 from dual where 1 = (select count(*) from frq_called_destinations
				where destination_type = 1
					and subscriber_id = 10253
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40    
					and avg_value = 10
					and destination = '+919845200002'   
					and network_id = 1024
					and profile_type = 'HOME');

select 1 from dual where 1 = (select count(*) from frq_called_destinations
				where destination_type = 1
					and subscriber_id = 10253
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40    
					and avg_value = 10
					and destination = '+919845200002'   
					and network_id = 1024
					and profile_type = 'ROAMER');

select 1 from dual where 1 = (select count(*) from frq_called_destinations
				where destination_type = 1
					and subscriber_id = 10253
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40    
					and avg_value = 10
					and destination = '+919845200002'   
					and network_id = 1024
					and profile_type = 'NRTRDE');

select 1 from dual where 1 = (select count(*) from frq_called_destinations
				where destination_type = 2
					and subscriber_id = 10253
					and phone_number = '+919845200005'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40    
					and avg_value = 10
					and destination = '+919845220002'   
					and network_id = 1024
					and profile_type = 'HOME');

select  1 from dual where 1 = (select count(*) from gprs_life_style_profiles
				where  phone_number = '+919845200005'
					and  subscriber_id  = 10253
					and profile_type = 'HOME' 
					and  subscriber_type = $FRAUD_SUBSCRIBER
					and  service_type = 2
					and  record_type = 1     
					and  cdr_count = 1      
					and  day_count = 1        
					and  total_value = 90        
					and  total_upload_volume = 500
					and  total_download_volume = 300);

select  1 from dual where 1 = (select count(*) from gprs_life_style_profiles
				where  phone_number = '+919845200005'
					and  subscriber_id  = 10253
					and profile_type = 'ROAMER' 
					and  subscriber_type = $FRAUD_SUBSCRIBER
					and  service_type = 2
					and  record_type = 1     
					and  cdr_count = 1      
					and  day_count = 1        
					and  total_value = 90        
					and  total_upload_volume = 300
					and  total_download_volume = 150);

select  1 from dual where 1 = (select count(*) from gprs_life_style_profiles
				where  phone_number = '+919845200005'
					and  subscriber_id  = 10253
					and profile_type = 'NRTRDE' 
					and  subscriber_type = $FRAUD_SUBSCRIBER
					and  service_type = 2
					and  record_type = 1     
					and  cdr_count = 1      
					and  day_count = 1        
					and  total_value = 90        
					and  total_upload_volume = 200
					and  total_download_volume = 100);

select  1 from dual where 1 = (select count(*) from subscriber_profile_dates 
	where subscriber_id  = 10253 and (to_number(to_char(date_of_generate, 'ddd')) = to_number(to_char(sysdate, 'ddd')))) ;
select 1 from dual where 5 = (select count(*) from blacklist_profile_options ) ;
select 1 from dual where 0 = (select count(*) from suspect_values ) ;
spool off
EOF

if [ $? -eq '5' ] || grep "no rows" lifestyleprofilegenerationblacklistsinglesubscriberV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f lifestyleprofilegenerationblacklistsinglesubscriberV.dat
exit $exitval
