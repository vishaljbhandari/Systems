FRAUD_SUBSCRIBER=1
FC_PHONE_NUMBER=2
FC_IMEI=3
FC_IMSI=4
FC_CELL_SITE_ID=14

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool lifestyleprofilegenerationblacklistV.dat
select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber where phone_number='+919845200001');
select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber where phone_number='+919845200008');
select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber where phone_number='+919845200009');
select 1 from dual where 1 = (select count(*) from account where id = 1027) ;
select 1 from dual where 5 = (select NVL(count(*), 0) from subscriber where subscriber_type = $FRAUD_SUBSCRIBER); 

select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber 
									where phone_number='+919845200011'
										and status = 3);

select 1 from dual where 1 = (select count(*) from usage_profiles
				where record_type = 1
					and call_type = 1
					and subscriber_id = 10252
					and phone_number = '+919845200001'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40                             
					and avg_value = 10); 

select 1 from dual where 1 = (select count(*) from frq_called_destinations
				where destination_type = 1
					and subscriber_id = 10252
					and phone_number = '+919845200001'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40    
					and avg_value = 10
					and destination = '+919845200002'   
					and network_id = 1024);

select 1 from dual where 1 = (select count(*) from frq_called_destinations
				where destination_type = 2
					and subscriber_id = 10252
					and phone_number = '+919845200001'                             
					and subscriber_type = $FRAUD_SUBSCRIBER
					and cdr_count = 1
					and avg_duration = 40    
					and avg_value = 10
					and destination = '+919845220002'   
					and network_id = 1024);

select 1 from dual where 1 = (select count(*) from suspect_values
				 where value = '404212000002'
					and source = 'Account Name : A.0000001040 Phone Number : +919845200001'
					and reason = 'FCN'
					and user_id = 2 
					and trunc(expiry_date) = trunc(sysdate+40)
					and entity_type = 3) ;

select 1 from dual where 1 = (select count(*) from audit_trails
				 where source = 7
					and event_code = 7
					and entity_type = 4000 
					and entity_value = '404212000002'
					and action = 'Hotlist Entity IMEI ''404212000002'' is added'
					and user_id = 'system'
					and network_id = '-' );
					 
select 1 from dual where 1 = (select count(*) from hotlist_groups_suspect_values 
				 where hotlist_group_id = (select id from hotlist_groups where name = 'Fraudulent') 
					and suspect_value_id = (select id from suspect_values where value = '404212000002'));

select 1 from dual where 1 = (select count(*) from suspect_values
				where value = '504220000001'
					and source = 'Account Name : A.0000001040 Phone Number : +919845200001'
					and reason = 'FCN'
					and user_id = 2
					and trunc(expiry_date) = trunc(sysdate+30)
					and entity_type = 4);

select 1 from dual where 1 = (select count(*) from audit_trails
				 where source = 7
					and event_code = 7
					and entity_type = 4000 
					and entity_value = '504220000001'
					and action = 'Hotlist Entity IMSI ''504220000001'' is added'
					and user_id = 'system'
					and network_id = '-' );
					 
select  1 from dual where 1 = (select count(*) from hotlist_groups_suspect_values
				 where hotlist_group_id = (select id from hotlist_groups where name = 'Fraudulent') 
					and suspect_value_id = (select id from suspect_values where value = '504220000001'));

select  1 from dual where 1 = (select count(*) from suspect_values
                where value = '404300004'
                    and trunc(expiry_date) = trunc(sysdate + 30)
                    and entity_type = $FC_CELL_SITE_ID);

select 1 from dual where 1 = (select count(*) from audit_trails
				 where source = 7
					and event_code = 7
					and entity_type = 4000 
					and entity_value = '404300004'
					and action = 'Hotlist Entity Cell Site Id ''404300004'' is added'
					and user_id = 'system'
					and network_id = '-');

select  1 from dual where 1 = (select count(*) from hotlist_groups_suspect_values
                 where hotlist_group_id = (select id from hotlist_groups where name = 'Suspected')
                    and suspect_value_id = (select id from suspect_values where value = '404300004'));

select  1 from dual where 1 = (select count(*) from gprs_life_style_profiles
				where  phone_number = '+919845200001'
					and  subscriber_id  = 10252
					and  subscriber_type = $FRAUD_SUBSCRIBER
					and  service_type = 2
					and  record_type = 1     
					and  cdr_count = 1      
					and  day_count = 1        
					and  total_value = 90        
					and  total_upload_volume = 500
					and  total_download_volume = 300);

select  1 from dual where 1 = (select count(*) from subscriber_profile_dates
				where subscriber_id  = 10252
					and (to_number(to_char(date_of_generate, 'ddd')) = to_number(to_char(sysdate, 'ddd')))) ;

select 1 from dual where 1 = (select count(*) from configurations where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024' and value = '28');

select value from configurations where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024' ;

select 1 from dual where 0 = (select count(*) from blacklist_profile_options where network_id = 1024) ;

select 1 from dual where 12 = (select count(*) from blacklist_profile_options where network_id = 1025) ;

spool off
EOF

if [ $? -eq '5' ] || grep "no rows" lifestyleprofilegenerationblacklistV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f lifestyleprofilegenerationblacklistV.dat
exit $exitval
