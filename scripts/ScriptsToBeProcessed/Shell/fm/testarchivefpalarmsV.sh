$SQL_COMMAND -s /nolog << EOF 
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool testarchivealarmsV.dat

select 1 from dual where 99999 = (select alarm_id from archive_details) ;
select 1 from dual where 99999 = (select id from archived_alarms where reference_type = 1 and reference_value = '+919820336303') ;
select 1 from dual where 99999 = (select alarm_id from archived_alerts) ;

select 1 from configurations 
where config_key = 'ARCHIVER.LAST_ARCHIVED_TIME' 
and trunc(to_date(value , 'yyyy/mm/dd hh24:mi:ss')) = trunc(sysdate) ;

select 1 from dual where 4 = (select count(*) from ARCHIVED_PROFILE_MATCH_INDEX) ; 
select 1 from dual where 4 = (select count(*) from AR_ALERT_PROFILE_MATCH_INDEX) ; 

spool off
EOF

SqlResult=$?

ls $COMMON_MOUNT_POINT/Attachments/ArchivedAlarm/99999*
FileResult=$?

if [ $SqlResult -ne  0 ] || [ $FileResult -ne  0 ] || grep "no rows" testarchivealarmsV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f testarchivealarmsV.dat
exit $exitval
