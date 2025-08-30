$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool testotherfraudV.dat

select 1 from dual where 1 = (select count(*) from fraudulent_info where
                                account_name = 'AAA'
                                AND phone_number = '+919820336303'
                                AND network_id = 1025
                                AND fraud_type = 'Prepay Fraud'
                                AND average_usage_per_day =0
                                AND trunc(alarm_created_date) = trunc(sysdate - 20)
                                AND trunc(alarm_modified_date) = trunc(sysdate-2)
                                AND alarm_value = 10
                                AND trunc(first_cdr_time) = trunc(sysdate-3)) ;

select 1 from dual where 99999 = (select value from configurations where config_key ='LAST_PROCESSED_BLACKLIST_ALARM_ID') ;

spool off
EOF

sqlret=$?

if [ "$sqlret" -ne "0" ] || grep "no rows" testotherfraudV.dat
then
    exitval=1
else
    exitval=0
fi

rm -f testotherfraudV.dat

exit $exitval
~
~
~
~
~
~

