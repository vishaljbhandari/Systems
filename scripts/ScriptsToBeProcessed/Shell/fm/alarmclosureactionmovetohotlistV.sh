FC_PHONE_NUMBER=2
FC_IMEI=3
FC_IMSI=4
FC_CELL_SITE_ID=14

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool alarmcloseractionhotlistV.dat

select 1 from dual where 1 = (select NVL(count(*), 0) from subscriber where phone_number='+919812400401');

select 1 from dual where 1 = (select count(*) from account where id = 1025) ;

select 1 from dual where 0 = (select count(*) from blacklist_profile_options ) ;

select 1 from dual where 1 = (select count(*) from hotlist_groups_suspect_values 
				 				where hotlist_group_id = (select id from hotlist_groups where name = 'Fraudulent') 
									and suspect_value_id = (select id from suspect_values where value = '+919812400401'));

select 1 from dual where 1 = (select count(*) from suspect_values
				where value = '+919812400401'
					and source = 'Account Name : 400401 Phone Number : +919812400401'
					and reason = 'FCN'
					and user_id = 2
					and trunc(expiry_date) = trunc(sysdate+50)
					and entity_type = $FC_PHONE_NUMBER);

select 1 from dual where 1 = (select count(*) from audit_trails
				 where source = 7
					and event_code = 7
					and entity_type = 4000 
					and entity_value = '+919812400401'
					and action = 'Hotlist Entity Phone Number ''+919812400401'' is added'
					and user_id = 'system'
					and network_id = '-');

select 1 from dual where 1 = (select count(*) from suspect_values
				where value = '45536100028787'
					and source = 'Account Name : 400401 Phone Number : +919812400401'
					and reason = 'FCN'
					and user_id = 2
					and trunc(expiry_date) = trunc(sysdate+40)
					and entity_type = $FC_IMEI);

select 1 from dual where 1 = (select count(*) from audit_trails
				 where source = 7
					and event_code = 7
					and entity_type = 4000 
					and entity_value = '45536100028787'
					and action = 'Hotlist Entity IMEI ''45536100028787'' is added'
					and user_id = 'system'
					and network_id = '-');

select 1 from dual where 1 = (select count(*) from suspect_values
				where value = '355361000284545'
					and source = 'Account Name : 400401 Phone Number : +919812400401'
					and reason = 'FCN'
					and user_id = 2
					and trunc(expiry_date) = trunc(sysdate+30)
					and entity_type = $FC_IMSI);

select 1 from dual where 1 = (select count(*) from audit_trails
				 where source = 7
					and event_code = 7
					and entity_type = 4000 
					and entity_value = '355361000284545'
					and action = 'Hotlist Entity IMSI ''355361000284545'' is added'
					and user_id = 'system'
					and network_id = '-');

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
				 where hotlist_group_id = (select id from hotlist_groups where name = 'Fraudulent') 
					and suspect_value_id = (select id from suspect_values where value = '+919812400401'));

select  1 from dual where 1 = (select count(*) from hotlist_groups_suspect_values
				 where hotlist_group_id = (select id from hotlist_groups where name = 'Fraudulent') 
					and suspect_value_id = (select id from suspect_values where value = '355361000284545'));

select  1 from dual where 1 = (select count(*) from hotlist_groups_suspect_values
				 where hotlist_group_id = (select id from hotlist_groups where name = 'Fraudulent') 
					and suspect_value_id = (select id from suspect_values where value = '45536100028787'));

select  1 from dual where 1 = (select count(*) from hotlist_groups_suspect_values
                 where hotlist_group_id = (select id from hotlist_groups where name = 'Suspected')
                    and suspect_value_id = (select id from suspect_values where value = '404300004'));


select 1 from dual where 1 = (select count(*) from blacklist_neural_profile) ;
select 1 from dual where 1 = (select count(*) from blacklist_neural_profile where blacklist_id=1025 and phone_number = '+919812400401') ;
select 1 from dual where 0 = (select count(*) from subscriber_neural_profile where subscriber_id=1025 and phone_number = '+919812400401') ;

select 1 from dual where 0 = (select count(*) from blacklist_profile_options) ;

select  1 from dual where 1 = (select count(*) from configurations 
		where config_key='ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1025' and value='5');

spool off
EOF

if [ $? -eq '5' ] || grep "no rows" alarmcloseractionhotlistV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f alarmcloseractionhotlistV.dat
exit $exitval
