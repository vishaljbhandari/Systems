$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
SET SERVEROUTPUT ON ;

delete from cdr;
delete from subscriber_profile_dates ;
delete from subscriber where id > 1024;
delete from account where id > 1024;

delete from hotlist_groups_suspect_values;
delete from suspect_values;
delete from gprs_life_style_profiles;
delete from hotlist_groups_networks ;
delete from hotlist_groups ;
delete from blacklist_profile_options;
delete from subscr_neural_voice_profile;
delete from subscr_neural_data_profile;
delete from blacklist_neural_profile;
delete from audit_trails ;

delete from configurations where config_key in ('HotList IMSI Expiry in Days', 'HotList IMEI Expiry in Days', 
												'HotList Cellsite Expiry in Days', 'HotList PhoneNumber Expiry in Days', 
												'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1025', 
												'ACA_STOP_PROCESSING_ST_1_PT_1_1025',
												'Top FCD Count') ;

delete from configurations where config_key = 'CLEANUP.RECORDS.CDR.OPTIONS' ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.CDR.OPTIONS', '01/01/1970 00:00:00,30', '0');


DROP SEQUENCE BLACKLIST_PROFILE_OPTIONS_SEQ ;
CREATE SEQUENCE BLACKLIST_PROFILE_OPTIONS_SEQ INCREMENT BY 1 NOMAXVALUE MINVALUE 1 NOCYCLE CACHE 20 ORDER ;

insert into configurations values (configurations_seq.nextval, 'Top FCD Count', '1', 0);

insert into hotlist_groups values (hotlist_groups_seq.nextval, 'Fraudulent',  '', 1,0) ;
insert into hotlist_groups_networks values (1024, hotlist_groups_seq.currval);
insert into hotlist_groups values (hotlist_groups_seq.nextval, 'Suspected',   '', 1,0) ;
insert into hotlist_groups_networks values (1024, hotlist_groups_seq.currval);

insert into configurations values (configurations_seq.nextval, 'HotList IMSI Expiry in Days', '30', 0);
insert into configurations values (configurations_seq.nextval, 'HotList IMEI Expiry in Days', '40', 0);
insert into configurations values (configurations_seq.nextval, 'HotList PhoneNumber Expiry in Days', '50', 0);
insert into configurations values (configurations_seq.nextval, 'HotList Cellsite Expiry in Days', '30', 0);

insert into configurations values (configurations_seq.nextval, 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1025', '1', 0);
insert into configurations values (configurations_seq.nextval, 'ACA_STOP_PROCESSING_ST_1_PT_1_1025', 'true', 0);

UPDATE configurations set value = '0' where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1025';

---- OUTGOING CDR
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number, day_of_year ) 
	values ( 1, 1025, '+919812400401', '+919845200001', null, 1, 40,  sysdate,
		'404212000002', '504220000002', '404300004', 1, 1025, 10, 1, 1, 1, 1, 0, 2, '+919812400401' , to_char(sysdate,'ddd')); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number, day_of_year ) 
	values ( 2, 1025, '+919845200003', '+919845200001', null, 1, 40,  sysdate, 
		'404212000002', '504220000002', '404300004', 1, 4, 10, 8, 1, 1, 1, 0, 2, '+919845200003', to_char(sysdate,'ddd'));

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number, day_of_year ) 
	values ( 3, 1025, '+919845200001', '+919812400401', null, 1, 40,  sysdate, 
		'404212000002', '504220000002', '404300004', 1, 1025, 10, 1, 1, 1, 1, 0, 2, '+919812400401' , to_char(sysdate,'ddd')); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number, day_of_year ) 
	values ( 4, 1025, '+919845200001', '+919812400401', null, 1, 40,  sysdate, 
		'404212000002', '504220000002', '404300004', 1, 10252, 10, 1, 1, 1, 1, 0, 2, '+919845200001' , to_char(sysdate,'ddd')); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number, day_of_year ) 
	values ( 3, 1025, '+919845200005', '+919812400401', null, 1, 40,  sysdate, 
		'404212000005', '504220000005', '404300004', 1, 10253, 10, 1, 1, 1, 1, 0, 2, '+919845200005' , to_char(sysdate,'ddd')); 

insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number, day_of_year ) 
	values ( 4, 1025, '+919845200005', '+919845200005', null, 1, 40,  sysdate, 
		'404212000005', '504220000005', '404300004', 1, 10253, 10, 1, 1, 1, 1, 0, 2, '+919845200005' , to_char(sysdate,'ddd')); 

COMMIT;

---- INCOMING CDR
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type, duration, time_stamp, equipment_id, 
		imsi_number, geographic_position, call_type, subscriber_id, value, cdr_type, service_type, cdr_category, is_complete, 
		is_attempted, service, phone_number, day_of_year ) 
	values ( 4, 1025, '+919845220002', '+919845200001', null, 2, 40,  sysdate, 
		'404212000002', '504220000002', '404300004', 1, 10252, 10, 1, 1, 1, 1, 0, 2, '+919845200001' , to_char(sysdate,'ddd')); 

COMMIT;

-- Account 
insert into account (id, account_type, account_name, account_doa, network_id, first_name, middle_name, last_name, address_line_1, 
		address_line_2, address_line_3, city, post_code) 
	values (1025, 1, '400401', to_date('01/01/2003', 'dd/mm/yyyy'),1025, 'ahmed' ,'abdul kader ali' ,'abdo' ,'aljzair', 
		'libia cente' ,'sana''a' ,'sana''a, yemen' ,'1848');

-- Subscriber 
insert into subscriber(id, subscriber_type, account_name, account_id, phone_number, subscriber_doa, home_phone_number, office_phone_number, 
		contact_phone_number, mcn1, mcn2, imsi, imei, connection_type, groups, services, status, qos, product_type, network_id) 
	values (1025, 0, '400401', 1025, '+919812400401', to_date('01/01/2003', 'dd/mm/yyyy'), '+918010000101','+918010000101', 
		'+918010000101','+918010000101','+918010000101','355361000284545','45536100028787','1','golden',4095, 3,1, 1, 1025) ;

COMMIT;

-- AI ROC Profile
insert into subscr_neural_voice_profile (subscriber_id, phone_number, sequence_id, network_id, 
									  profile_type, profile_matured, maturity_info, 
									  neural_profile1, neural_profile2, neural_profile3, neural_profile4)
	values (1025, '+919812400401', 1, 1025, 3, 0, 'MI', 'NP1', 'NP2', 'MP3', 'NP4') ;

insert into subscr_neural_voice_profile (subscriber_id, phone_number, sequence_id, network_id, 
									  profile_type, profile_matured, maturity_info, 
									  neural_profile1, neural_profile2, neural_profile3, neural_profile4)
	values (1026, '+919845000002', 1, 1024, 3, 0, 'MI', 'NP1', 'NP2', 'MP3', 'NP4') ;

COMMIT;

declare 
	i 		number(10,0):=0 ;	
begin
	for i in 2..5 loop
		insert into blacklist_profile_options(id, subscriber_id, phone_number, option_id, network_id)
				select blacklist_profile_options_seq.nextval, id, PHONE_NUMBER, i, network_id from subscriber where id = 1025;
	end loop;

	insert into blacklist_profile_options 
			select blacklist_profile_options_seq.nextval, id, PHONE_NUMBER, 14, network_id from subscriber where id = 1025;

	commit;
end;	
/

EOF
