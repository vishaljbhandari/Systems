$SQL_COMMAND /nolog <<EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from account where id = 1700;
delete from subscriber where id > 4 ;
delete from subscriber_groups_summary ;

insert into account (ID, ACCOUNT_TYPE, ACCOUNT_NAME, FRD_INDICATOR, GROUPS, NETWORK_ID) 
		values (1700, 1, 'JAY_1700_JAY', 0, 'Default', 1025);

------------------------------------------------------------------------------------------------------------------------
		
insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1400, 1, 1700, '+911400', sysdate-200, 1, 'MVN,PVN,BAN,AGN,CAN', 0, 1, sysdate-20, 1025); 
		
insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1401, 1, 1700, '+911401', sysdate-200, 1, 'MVN,PVN', 0, 1, sysdate-20, 1025); 

insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1402, 1, 1700, '+911402', sysdate-200, 1, 'BAN,AGN,CAN', 0, 1, sysdate-20, 1025); 

insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1403, 1, 1700, '+911403', sysdate-200, 1, 'PVN,BAN', 0, 1, sysdate-20, 1025); 

insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1404, 1, 1700, '+91140', sysdate-200, 1, 'MVN,BAN,AGN,CAN', 0, 1, sysdate-20, 1026); 
		
insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1405, 1, 1700, '+911401', sysdate-200, 1, 'MVN,PVN,AGN', 0, 1, sysdate-20, 1026); 

insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1406, 1, 1700, '+911402', sysdate-200, 1, 'BAN,CAN', 0, 1, sysdate-20, 1026); 

insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1407, 1, 1700, '+911403', sysdate-200, 1, 'PVN,AGN', 0, 1, sysdate-20, 1026); 

insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1408, 1, 1700, '+911401', sysdate-200, 1, 'MVN,PVN,AGN', 0, 1, sysdate-20, 1027); 

insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1409, 1, 1700, '+911402', sysdate-200, 1, 'CAN', 0, 1, sysdate-20, 1027); 

insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1410, 1, 1700, '+911402', sysdate-200, 1, 'Default', 0, 1, sysdate-20, 1025); 

insert into subscriber (ID, SUBSCRIBER_TYPE, ACCOUNT_ID, PHONE_NUMBER, SUBSCRIBER_DOA, CONNECTION_TYPE, GROUPS, 
		STATUS, PRODUCT_TYPE, MODIFIED_DATE, NETWORK_ID)
		values (1411, 1, 1700, '+911402', sysdate-200, 1, 'Default', 0, 1, sysdate-20, 1025);
		
commit;
EOF

if [ $? -ne 0 ];then
	exit 1
fi

