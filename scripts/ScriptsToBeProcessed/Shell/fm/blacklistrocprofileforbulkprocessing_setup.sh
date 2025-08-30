$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set serveroutput on ;

drop sequence blacklist_profile_options_seq ;
create sequence blacklist_profile_options_seq increment by 1 nomaxvalue minvalue 1 nocycle cache 20 order ;

delete from subscriber_profile_dates ;
delete from subscriber where id > 1024 ;
delete from account where id > 1024 ;
delete from blacklist_profile_options ;
delete from subscr_neural_data_profile ;
delete from subscr_neural_voice_profile ;
delete from blacklist_neural_profile ;
delete from audit_trails ;
delete from configurations where config_key in (	'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024',
													'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024',
													'ACA_STOP_PROCESSING_ST_1_PT_1_1024'  ) ; 


insert into configurations values (configurations_seq.nextval, 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024', '1', '0');
insert into configurations values (configurations_seq.nextval, 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024', '0', '0');
insert into configurations values (configurations_seq.nextval, 'ACA_STOP_PROCESSING_ST_1_PT_1_1024', 'true', '0');

insert into account (id, account_type, account_name, account_doa, network_id, 
					 first_name, middle_name, last_name, 
					 address_line_1, address_line_2, address_line_3, city, post_code) 
	     values 
					(1025, 1, 'A.0000001040', to_date('01/01/2003', 'dd/mm/yyyy'), 1024, 
					 'Abdul', 'Kader', 'Mamu',
					 '' ,'' ,'' ,'','1848');

insert into subscriber (id, subscriber_type, account_name, account_id, phone_number, 
						subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, 
						mcn2, imsi, imei, connection_type, groups, 
						services, status, qos, product_type, network_id) 
				values 
						(10251, 1, 'A.0000001040', 1025, '+919845000001', 
						 to_date('01/01/2003', 'dd/mm/yyyy'), '97111025' , '97121025' , '97131025' , '97141025', 
						 '97151025' , '504220000005' , '404212000005' , '1' , 'golden', 
						 4095, 1, 1, 1, 1024) ;

insert into subscriber (id, subscriber_type, account_name, account_id, phone_number, 
						subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, 
						mcn2, imsi, imei, connection_type, groups, 
						services, status, qos, product_type, network_id) 
				values 
						(10252, 1, 'A.0000001040', 1025, '+919845000002', 
						 to_date('01/01/2003', 'dd/mm/yyyy'), '97111025' , '97121025' , '97131025' , '97141025', 
						 '97151025' , '504220000005' , '404212000005' , '1' , 'golden', 
						 4095, 1, 1, 1, 1024) ;

insert into subscriber (id, subscriber_type, account_name, account_id, phone_number, 
						subscriber_doa, home_phone_number, office_phone_number, contact_phone_number, mcn1, 
						mcn2, imsi, imei, connection_type, groups, 
						services, status, qos, product_type, network_id) 
				values 
						(10253, 0, 'A.0000001040', 1025, '+919845000003', 
						 to_date('01/01/2003', 'dd/mm/yyyy'), '97111025' , '97121025' , '97131025' , '97141025', 
						 '97151025' , '504220000005' , '404212000005' , '1' , 'golden', 
						 4095, 1, 1, 1, 1024) ;

insert into subscr_neural_voice_profile (subscriber_id, phone_number, sequence_id, network_id, 
									  profile_type, profile_matured, maturity_info, 
									  neural_profile1, neural_profile2, neural_profile3, neural_profile4)
values
		(10251, '+919845000001', 1, 1024, 3, 0, 'MI', 'NP1', 'NP2', 'MP3', 'NP4') ;

insert into subscr_neural_voice_profile (subscriber_id, phone_number, sequence_id, network_id, 
									  profile_type, profile_matured, maturity_info, 
									  neural_profile1, neural_profile2, neural_profile3, neural_profile4)
values
		(10253, '+919845000002', 1, 1024, 3, 0, 'MI', 'NP1', 'NP2', 'MP3', 'NP4') ;

update configurations set value = '0' where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024' ;
update configurations set value = '0' where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024' ;

insert into blacklist_profile_options select blacklist_profile_options_seq.nextval, id, phone_number, 5, network_id from subscriber where subscriber_type = 1 ;

commit;
/
EOF
