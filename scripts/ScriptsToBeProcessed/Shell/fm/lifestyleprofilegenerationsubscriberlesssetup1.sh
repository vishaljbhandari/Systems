$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from cdr;

delete from subscriber_profile_dates ;
delete from usage_profiles;
delete from frq_called_destinations;
delete from blacklist_profile_options ;
delete from gprs_life_style_profiles;
delete subscriber where id > 1024 ;
delete account where id > 1024 ;
delete from configurations where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024';
delete from configurations where config_key = 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024';
delete from hotlist_groups_suspect_values;
delete from hotlist_groups_networks ;
delete from hotlist_groups ;
delete from suspect_values;
delete from audit_trails;

delete from configurations where config_key = 'CLEANUP.RECORDS.CDR.OPTIONS' ;
insert into configurations values (configurations_seq.nextval, 'CLEANUP.RECORDS.CDR.OPTIONS', '01/01/1970 00:00:00,30', '0');

insert into configurations values (configurations_seq.nextval, 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_0_PT_0_1024', '0', '0');
insert into configurations values (configurations_seq.nextval, 'ACA_LAST_PROCESSED_LSP_BLK_ID_ST_1_PT_1_1024', '0', '0');

insert into hotlist_groups values (hotlist_groups_seq.nextval, 'Fraudulent',  '', 1, 0) ;
insert into hotlist_groups_networks values (1024, hotlist_groups_seq.currval);
insert into hotlist_groups values (hotlist_groups_seq.nextval, 'Suspected',   '', 1, 0) ;
insert into hotlist_groups_networks values (1024, hotlist_groups_seq.currval);

DROP SEQUENCE BLACKLIST_PROFILE_OPTIONS_SEQ ;
CREATE SEQUENCE BLACKLIST_PROFILE_OPTIONS_SEQ INCREMENT BY 1 NOMAXVALUE MINVALUE 1 NOCYCLE CACHE 20 ORDER ;

INSERT INTO SUBSCRIBER(ID,SUBSCRIBER_TYPE, ACCOUNT_NAME, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, HOME_PHONE_NUMBER,
	OFFICE_PHONE_NUMBER, CONTACT_PHONE_NUMBER, MCN1, MCN2, IMSI, IMEI, CONNECTION_TYPE, GROUPS, SERVICES, STATUS, QOS,
	PRODUCT_TYPE, NETWORK_ID) VALUES (1025, 1, null, 4, '+919845200003', TO_DATE('01/01/2003', 'DD/MM/YYYY'),'97111025' ,'97121025' ,'97131025' ,'97141025' ,'97151025' ,'504220000001' ,'404212000002' ,'1' ,'GOLDEN' ,4095, 0,1, 1, 1024) ;
commit;
 
insert into cdr ( id, network_id, caller_number, called_number, forwarded_number, record_type,
duration, time_stamp, equipment_id, imsi_number, geographic_position, call_type, subscriber_id,
value, cdr_type, service_type, cdr_category, is_complete, is_attempted, service, phone_number,
day_of_year ) values ( 2, 1024, '+919845200003', '+919845200001', null, 1, 40,sysdate
, '404212000002', '504220000002', '', 1, 1025, 10, 8, 1, 1, 1, 0, 2, '+919845200003'
, 224); 

update cdr set day_of_year = to_number(to_char(time_stamp,'ddd')) ;

commit;

declare 
	i 		number(10,0):=0 ;	
begin
	for i in 1..5 loop
		insert into blacklist_profile_options(id, subscriber_id, phone_number, option_id, network_id)
			select BLACKLIST_PROFILE_OPTIONS_SEQ.nextval, id, PHONE_NUMBER, i, network_id from subscriber 
				where subscriber_type = 1;
	end loop;			
	insert into blacklist_profile_options select BLACKLIST_PROFILE_OPTIONS_SEQ.nextval, id, PHONE_NUMBER, 14, network_id from subscriber where status = 0;
	commit;
end;	
/
EOF
if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

exit $exitval
