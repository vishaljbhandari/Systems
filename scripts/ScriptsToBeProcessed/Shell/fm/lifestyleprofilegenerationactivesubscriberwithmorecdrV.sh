ACTIVE_SUBSCRIBER=0
FC_PHONE_NUMBER=2
FC_IMEI=3
FC_IMSI=4

$SQL_COMMAND /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool lifestyleprofilegenerationactivesubscriberwithmorecdrV.dat

select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber where phone_number='+919845200005');
select 1 from dual where 1 = (select count(*) from account where id = 1025) ;
select 1 from dual where 1 = (select count(*) from subscriber where subscriber_type = $ACTIVE_SUBSCRIBER and id = 5200); 

select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber 
									where phone_number='+919845200005'
										and subscriber_type = $ACTIVE_SUBSCRIBER);

select 1 from dual where 1 = (select count(*) from usage_profiles
				where record_type = 2
					and call_type = 1
					and subscriber_id = 5200
					and profile_type = 'HOME'
					and phone_number = '+919845200005'                             
					and vpmn = '111111'
					and subscriber_type = $ACTIVE_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40                             
					and avg_value = 10); 

select 1 from dual where 1 = (select count(*) from usage_profiles
				where record_type = 2
					and call_type = 2
					and subscriber_id = 5200
					and profile_type = 'HOME'
					and phone_number = '+919845200005'                             
					and vpmn = '111111'
					and subscriber_type = $ACTIVE_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40                             
					and avg_value = 10); 

select 1 from dual where 1 = (select count(*) from usage_profiles
				where record_type = 2
					and call_type = 3
					and subscriber_id = 5200
					and profile_type = 'HOME'
					and phone_number = '+919845200005'                             
					and vpmn = '111111'
					and subscriber_type = $ACTIVE_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40                             
					and avg_value = 10); 

select 1 from dual where 1 = (select count(*) from usage_profiles
				where record_type = 2
					and call_type = 3
					and subscriber_id = 5200
					and profile_type = 'ROAMER'
					and phone_number = '+919845200005'                             
					and vpmn = '111111'
					and subscriber_type = $ACTIVE_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40                             
					and avg_value = 10); 

select 1 from dual where 1 = (select count(*) from usage_profiles
				where record_type = 2
					and call_type = 3
					and subscriber_id = 5200
					and profile_type = 'NRTRDE'
					and phone_number = '+919845200005'                             
					and vpmn = '111111'
					and subscriber_type = $ACTIVE_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40                             
					and avg_value = 10); 

select 1 from dual where 1 = (select count(*) from frq_called_destinations
				where destination_type = 2
					and subscriber_id = 5200
					and phone_number = '+919845200005'                             
					and subscriber_type = $ACTIVE_SUBSCRIBER
					and cdr_count = 3
					and avg_duration = 40    
					and avg_value = 10
					and destination = '+919845200001'
					and network_id = 1024
					and profile_type = 'HOME');

select 1 from dual where 1 = (select count(*) from frq_called_destinations
				where destination_type = 5
					and subscriber_id = 5200
					and phone_number = '+919845200005'                             
					and subscriber_type = $ACTIVE_SUBSCRIBER
					and cdr_count = 3
					and avg_duration = 40    
					and avg_value = 10
					and destination = '+91'   
					and network_id = 1024
					and profile_type = 'HOME');

select  1 from dual where 1 = (select count(*) from gprs_life_style_profiles
				where  phone_number = '+919845200005'
					and  subscriber_id  = 5200
					and  subscriber_type = $ACTIVE_SUBSCRIBER
					and profile_type = 'HOME'
					and  service_type = 1
					and  record_type = 2     
					and  cdr_count = 1      
					and  day_count = 1        
					and  total_value = 90        
					and  total_upload_volume = 500
					and  total_download_volume = 300);

select  1 from dual where 1 = (select count(*) from gprs_life_style_profiles
				where  phone_number = '+919845200005'
					and  subscriber_id  = 5200
					and  subscriber_type = $ACTIVE_SUBSCRIBER
					and profile_type = 'ROAMER'
					and  service_type = 1
					and  record_type = 2     
					and  cdr_count = 1      
					and  day_count = 1        
					and  total_value = 90        
					and  total_upload_volume = 200
					and  total_download_volume = 200);

select  1 from dual where 1 = (select count(*) from gprs_life_style_profiles
				where  phone_number = '+919845200005'
					and  subscriber_id  = 5200
					and  subscriber_type = $ACTIVE_SUBSCRIBER
					and profile_type = 'NRTRDE'
					and  service_type = 1
					and  record_type = 2     
					and  cdr_count = 1      
					and  day_count = 1        
					and  total_value = 90        
					and  total_upload_volume = 150
					and  total_download_volume = 250);

select  1 from dual where 1 = (select count(*) from subscriber_profile_dates
				where subscriber_id  = 5200
					and (to_number(to_char(date_of_generate, 'ddd')) = to_number(to_char(sysdate, 'ddd')))) ;

spool off
EOF

if [ $? -eq '5' ] || grep "no rows" lifestyleprofilegenerationactivesubscriberwithmorecdrV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f lifestyleprofilegenerationactivesubscriberwithmorecdrV.dat
exit $exitval
