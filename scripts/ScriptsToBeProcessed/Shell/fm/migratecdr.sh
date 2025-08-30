#!/bin/bash
#Usage ./migratecdr.sh <DAY_OF_YEAR>
#Requires Read Access from the ranger Schema on newly created Nikira Database

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

PerformCDRMigration()
{
sqlplus -l -s /nolog <<EOF
connect $NIKIRA_DB_USER/$NIKIRA_DB_PASSWORD
set feedback on;
set serveroutput on;
set time on;
set timing on;
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

spool CDR_MIGRATION_$DAY_OF_YEAR.log ;

DECLARE
	subpart_suffix	VARCHAR2(10) ;
	subpartnum	NUMBER(20) ;
BEGIN

	FOR i in 1..(32)
	LOOP
		IF i < 10  THEN
			subpart_suffix := '0'||i ;
		END IF ;

		subpartnum := mod(i,24) ;

		execute immediate 'insert /*+APPEND*/ into cdr SUBPARTITION(P$DAY_OF_YEAR_PREFIX'||'_SP_'||subpartnum-1||') 
						(id,network_id,caller_number,called_number,forwarded_number,record_type,duration,time_stamp,equipment_id,imsi_number,
							geographic_position,call_type,subscriber_id,value,cdr_type,service_type,cdr_category,is_complete,is_attempted,
							service,phone_number,vpmn,day_of_year,source,is_conference,is_subscriberless,cell_site_name,in_charge,hour_of_day)
				values ( select id,999,caller_number,called_number,forwarded_number,record_type,duration,time_stamp, equipment_id,imsi_number,
						geographic_position,call_type,subscriber_id,value,cdr_type,service_type,cdr_category,is_complete,is_attempted,
						service,phone_number,vpmn,day_of_year,source,is_conference,is_subscriberless,cell_site_name,in_charge,'||subpartnum||
						' from $RANGER_DB_USER.cdr subpartition(P$DAY_OF_YEAR_PREFIX'||'_TH'||subpart_suffix||)') ';

		DBMS_OUTPUT.put_line('Completed CDR migration for Day $DAY_OF_YEAR SubPartition P$DAY_OF_YEAR_PREFIX'||'_TH'||subpart_suffix) ;

	END LOOP;
END;
/

spool off ;
quit ;
EOF

if [ $? -ne 0 ] ; then
	echo "CDR Migration failed for Day $DAY_OF_YEAR"
	return 1
fi

echo "CDR Migration successfully completed for Day $DAY_OF_YEAR"

PerformCDRPartitionTruncation
}

PerformCDRPartitionTruncation()
{
sqlplus -l -s /nolog <<EOF
connect $NIKIRA_DB_USER/$NIKIRA_DB_PASSWORD
set feedback on;
set serveroutput on;
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

spool CDR_TRUNCATION_$DAY_OF_YEAR.log ;

alter table cdr truncate partition P$DAY_OF_YEAR_PREFIX drop storage ;

spool off;
quit ;
EOF

echo "Truncated CDR table for Day $DAY_OF_YEAR"
}

GetDayOfYearPrefix
PerformCDRMigration
