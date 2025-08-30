#! /bin/sh

STATUS_SPOOLED=3
RULE_ID=7000

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL

whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool negativeruleexecutionV.dat

select 1 from dual where 1 = ( select count(*) from negative_rule_exec_status where status = $STATUS_SPOOLED ) ;

select 1 from dual where 1 = ( select count(*) from negative_rule_exec_status where to_date(start_time, 'DD/MM/YYYY') = to_date(sysdate, 'DD/MM/YYYY')) ;

select 1 from dual where 1 = ( select count(*) from negative_rule_exec_status where to_date(end_time, 'DD/MM/YYYY') = to_date(sysdate, 'DD/MM/YYYY')) ;

select 1 from dual where 1 = ( select count(*) from negative_rule_exec_status where status = $STATUS_SPOOLED) ;

select 1 from dual where 0 = ( select count(*) from temp_negative_rule_data where rule_id = $RULE_ID ) ;

spool off
EOF

[ -d $COMMON_MOUNT_POINT/FMSData/NegativeRuleData/Process ] && echo "NoOfFiles: `ls -l $COMMON_MOUNT_POINT/FMSData/NegativeRuleData/Process/NEG* | wc -l`" >> negativeruleexecutionV.dat
[ -d $COMMON_MOUNT_POINT/FMSData/NegativeRuleData/Process/success ] && echo "NoOfFilesInSuccess: `ls -l $COMMON_MOUNT_POINT/FMSData/NegativeRuleData/Process/success/NEG* | wc -l`" >> negativeruleexecutionV.dat

if [ $? -eq '5' ] || grep "no rows" negativeruleexecutionV.dat || ! grep "NoOfFiles: 1" negativeruleexecutionV.dat || ! grep "NoOfFilesInSuccess: 1" negativeruleexecutionV.dat
then
	exitval=1
else
	exitval=0
fi

rm -f negativeruleexecutionV.dat
rm -f $COMMON_MOUNT_POINT/FMSData/NegativeRuleData/Process/NEG*.txt
rm -f $COMMON_MOUNT_POINT/FMSData/NegativeRuleData/Process/success/NEG*.txt
rm -f spooled_data_1.txt
exit $exitval
