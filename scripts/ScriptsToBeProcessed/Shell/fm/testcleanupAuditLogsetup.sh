$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from configurations where config_key='CLEANUP.AUDITLOG.INTERVAL_IN_DAYS' ;
delete from configurations where config_key='CLEANUP.AUDITLOG.LAST_RUN_DATE' ;

insert into configurations(id, config_key, value) values (configurations_seq.nextval,'CLEANUP.AUDITLOG.INTERVAL_IN_DAYS', '30');
insert into configurations(id, config_key, value) values (configurations_seq.nextval,'CLEANUP.AUDITLOG.LAST_RUN_DATE', '1970/01/01 00:00:00') ;


delete from audit_trails;
delete from audit_files_processed;

insert into audit_trails values(1,'1024,1025,1026,1027,1028',sysdate-90,18,636,9013,'nadmin',3,'User ''nadmin'' logged in','127.0.0.1','nadmin', 'nadmin') ;
insert into audit_trails values(2,'1024,1025,1026,1027,1028',sysdate,18,637,9013,'nadmin',3,'User ''nadmin'' logged out','127.0.0.1','nadmin', 'nadmin') ;

insert into audit_files_processed values(1, 'Subscriber','Processed','$COMMON_MOUNT_POINT/FMSData/SubscriberDataRecord/Process/subscriber',sysdate-90,sysdate-90,1,1,0);
insert into audit_files_processed values(2, 'CDR','Processed','$COMMON_MOUNT_POINT/FMSData/SubscriberDataRecord/Process/cdr',sysdate,sysdate,1,1,0);

commit;
EOF

if [ $? -ne 0 ];then
	exit 1
fi

