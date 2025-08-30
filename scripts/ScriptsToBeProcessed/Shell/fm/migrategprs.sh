#!/bin/bash
#Usage ./migrategprs.sh <DAY_OF_YEAR>
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

PerformGPRSMigration()
{
sqlplus -l -s /nolog <<EOF
connect $NIKIRA_DB_USER/$NIKIRA_DB_PASSWORD
set feedback on;
set serveroutput on;
set time on;
set timing on;
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

spool GPRS_MIGRATION_$DAY_OF_YEAR.log ;

DECLARE
	subpart_suffix	VARCHAR2(10) ;
	subpartnum	NUMBER(20) ;
BEGIN

	FOR i in 1..(8)
	LOOP
		IF i < 10  THEN
			subpart_suffix := '0'||i ;
		END IF ;

		subpartnum := i ;

		execute immediate 'insert /*+APPEND*/ into gprs_cdr SUBPARTITION(P$DAY_OF_YEAR_PREFIX'||'_SP_'||subpartnum-1||') 
						(id,network_id,record_type,time_stamp,duration,phone_number,imsi_number,imei_number,geographic_position,cdr_type,service_type,
						pdp_type,served_pdp_address,upload_data_volume,download_data_volume,service,qos_requested,qos_negotiated,value,access_point_name,
						subscriber_id,day_of_year,cause_for_session_closing,session_status,charging_id,ani,sender_terminal_type,sender_terminal_ip_address,
						orig_vasp_id,orig_vas_id,sender_roaming_class,recipient_terminal_ip_address,address_hiding_status,dest_vasp_id,dest_vas_id,
						recipient_roaming_class,forwarded_by,msg_id,Priority,cnt_size_video,cnt_number_video,cnt_size_audio,cnt_number_audio,cnt_size_image,
						cnt_number_image,cnt_size_text,cnt_number_text,cnt_size_other,cnt_number_other,recipient_number,no_of_successful_recipients,dest_mmsc,delivery_status,error_cause,tariff_class,message_class,sender_charging_type,recipient_charging_type,sender_prepaid_status,recipient_prepaid_status,other_party_number,source,cell_site_name,in_charge,hour_of_day)
					values
						(select id,network_id,record_type,time_stamp,duration,phone_number,imsi_number,imei_number,geographic_position,cdr_type,service_type,
						pdp_type,served_pdp_address,upload_data_volume,download_data_volume,service,qos_requested,qos_negotiated,value,access_point_name,
						subscriber_id,day_of_year,cause_for_session_closing,session_status,charging_id,ani,sender_terminal_type,sender_terminal_ip_address,
						orig_vasp_id,orig_vas_id,sender_roaming_class,recipient_terminal_ip_address,address_hiding_status,dest_vasp_id,dest_vas_id,
						recipient_roaming_class,forwarded_by,msg_id,Priority,cnt_size_video,cnt_number_video,cnt_size_audio,cnt_number_audio,cnt_size_image,
						cnt_number_image,cnt_size_text,cnt_number_text,cnt_size_other,cnt_number_other,recipient_number,no_of_successful_recipients,dest_mmsc,delivery_status,error_cause,tariff_class,message_class,sender_charging_type,recipient_charging_type,sender_prepaid_status,recipient_prepaid_status,other_party_number,source,cell_site_name,in_charge ,'||subpartnum||
						' from $RANGER_DB_USER.gprs_cdr subpartition(P$DAY_OF_YEAR_PREFIX'||'_TH'||subpart_suffix||)') ';

		DBMS_OUTPUT.put_line('Completed GPRS_CDR migration for Day $DAY_OF_YEAR SubPartition P$DAY_OF_YEAR_PREFIX'||'_TH'||subpart_suffix) ;

	END LOOP;
END;
/

spool off ;
quit ;
EOF

if [ $? -ne 0 ] ; then
	echo "GPRS_CDR Migration failed for Day $DAY_OF_YEAR"
	return 1
fi

echo "GPRS_CDR Migration successfully completed for Day $DAY_OF_YEAR"

PerformGPRSPartitionTruncation
}

PerformGPRSPartitionTruncation()
{
sqlplus -l -s /nolog <<EOF
connect $NIKIRA_DB_USER/$NIKIRA_DB_PASSWORD
set feedback on;
set serveroutput on;
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;

spool GPRS_TRUNCATION_$DAY_OF_YEAR.log ;

alter table GPRS_CDR truncate partition P$DAY_OF_YEAR_PREFIX drop storage ;

spool off;
quit ;
EOF

echo "Truncated GPRS_CDR table for Day $DAY_OF_YEAR"
}

GetDayOfYearPrefix
PerformGPRSMigration
