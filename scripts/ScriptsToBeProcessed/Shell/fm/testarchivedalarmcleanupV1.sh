$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool testarchivedalarmcleanupnV1.dat

select 1 from dual where 1 = (select count(*) from ARCHIVED_ALARMS ) ;
select 1 from dual where 1 = (select count(*) from ARCHIVED_ALERTS ) ;
select 1 from dual where 1 = (select count(*) from ARCHIVED_ALARM_COMMENTS ) ;
select 1 from dual where 1 = (select count(*) from ARCHIVED_ALARMS_FRAUD_TYPES ) ;
select 1 from dual where 3 = (select count(*) from ARCHIVED_ALERT_CDR ) ;
select 1 from dual where 1 = (select count(*) from ARCHIVED_CDR ) ;
select 1 from dual where 1 = (select count(*) from ARCHIVED_GPRS_CDR ) ;
select 1 from dual where 1 = (select count(*) from ARCHIVED_MATCHED_DETAILS ) ;
select 1 from dual where 1 = (select count(*) from ARCHIVED_MATCHED_RESULTS ) ;
select 1 from dual where 1 = (select count(*) from AR_MULTI_FIELD_FUNC_RES_INFO ) ;
select 1 from dual where 1 = (select count(*) from ARCHIVED_RECHARGE_LOG ) ;
select 1 from dual where trunc(sysdate) = (select to_date(value,'YYYY/MM/DD HH24:MI:SS') from configurations where config_key = 'CLEANUP.ARCHIVED.ALARMS.LAST_RUN_DATE') ;

spool off
commit ;
quit ;
EOF

if [ $? -ne  '0' ] || grep "no rows" testarchivedalarmcleanupnV1.dat
then
	exitval=1
else
	exitval=0
fi

if test -f $COMMON_MOUNT_POINT/Attachments/ArchivedAlarm/99999ABC.txt
then
	exitval=2
fi

rm -f testarchivedalarmcleanupnV1.dat
exit $exitval
