#!/bin/bash
#Usage ./migraterecharge.sh <DAY_OF_YEAR>
#Requires Read Access from the ranger Schema on newly created Nikira Database
#Migrates 5.4 Recharge_log table Which has no Subpartitions to 7.0 Recharge_log which has 4 subpartitions .using mod(id,4)

. $RANGERHOME/sbin/rangerenv.sh

DAY_OF_YEAR=$1
NIKIRA_DB_USER=nikira
NIKIRA_DB_PASSWORD=nikira
RANGER_DB_USER=ranger

GetDayOfYearPrefix()
{
	DAY_OF_YEAR_PREFIX="$DAY_OF_YEAR"

	if [ $DAY_OF_YEAR -lt 9 ] ; then
		DAY_OF_YEAR_PREFIX="00$DAY_OF_YEAR"
	fi

	if [ "$DAY_OF_YEAR -gt 9" -a "$DAY_OF_YEAR -lt 100" ] ; then
		DAY_OF_YEAR_PREFIX="0$DAY_OF_YEAR"
	fi

}

PerformRechargeMigration()
{
sqlplus -l -s /nolog <<EOF
connect $NIKIRA_DB_USER/$NIKIRA_DB_PASSWORD
set feedback on;
set serveroutput on;
set time on;
set timing on;
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

spool RECHARGE_MIGRATION_$DAY_OF_YEAR.log ;

DECLARE
BEGIN
	execute immediate 'insert /*+APPEND*/ into recharge_log PARTITION(P$DAY_OF_YEAR_PREFIX) 
					(id,time_stamp,phone_number,amount,recharge_type,status,credit_card,pin_number,serial_number,network_id,account_id,subscriber_id,
					cdr_type,acct_id,acct_ref_id,du_user,account_type,balance_types,balances,acs_cust_id,bonus_type,cs,PI,reference,result_reason,
					wallet_type,batch_description,type_description,voucher,nack,redeeming_acct_ref,host,voucher_type,badpins,charge_name,tcs,event_class,
					event_name,event_cost,event_time_cost,event_count,discount,cascade,day_of_year,hour_of_day)
				values
				(select id,time_stamp,phone_number,amount,recharge_type,status,credit_card,pin_number,serial_number,network_id,account_id,subscriber_id,
					cdr_type,acct_id,acct_ref_id,du_user,account_type,balance_types,balances,acs_cust_id,bonus_type,cs,PI,reference,result_reason,
					wallet_type,batch_description,type_description,voucher,nack,redeeming_acct_ref,host,voucher_type,badpins,charge_name,tcs,event_class,
					event_name,event_cost,event_time_cost,event_count,discount,cascade,day_of_year,mod(id,4)+1
					 from $RANGER_DB_USER.recharge_log partition(P$DAY_OF_YEAR_PREFIX))';

	DBMS_OUTPUT.put_line('Completed RECHARGE_LOG migration for Day $DAY_OF_YEAR Partition P$DAY_OF_YEAR_PREFIX') ;
END;
/

spool off ;
quit ;
EOF

if [ $? -ne 0 ] ; then
	echo "RECHARGE_LOG Migration failed for Day $DAY_OF_YEAR"
	return 1
fi

echo "RECHARGE_LOG Migration successfully completed for Day $DAY_OF_YEAR"

PerformRechargePartitionTruncation
}

PerformRechargePartitionTruncation()
{
sqlplus -l -s /nolog <<EOF
connect $NIKIRA_DB_USER/$NIKIRA_DB_PASSWORD
set feedback on;
set serveroutput on;
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

spool RECHARGE_TRUNCATION_$DAY_OF_YEAR.log ;

alter table RECHARGE_LOG truncate partition P$DAY_OF_YEAR_PREFIX drop storage ;

spool off;
quit ;
EOF

echo "Truncated RECHARGE_LOG table for Day $DAY_OF_YEAR"
}

GetDayOfYearPrefix
PerformRechargeMigration
