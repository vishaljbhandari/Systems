#!/bin/bash

TskId=$1
ParentTrendSummaryName=$2
ReportWeekRetention=$3
ParentTSLastFromDTTMWeek=$4
ParentTSLastToDTTMWeek=$5
ParentTSLastFromDTTM=$6
ParentTSLastToDTTM=$7
vsql_cmd=`which vsql`
float_format=$8
vertica_username=$9
vertica_password=${10}
databasename=${11}
vertica_ip=${12}
output_file="$COMMON_MOUNT_POINT/LOG/vertica_trendSummaryScript_$TskId.log"
>$output_file

function executeCmdInVerticaDB()
{

query=$1
query_with_commit=$query"commit;"
stageName=$2
isCommit=$3
finalQuery=$query
if [ $isCommit ]
then
	finalQuery=$query_with_commit
fi
`$vsql_cmd -At -h "$vertica_ip" -d "$databasename" -U "$vertica_username" -w "$vertica_password" -c "$finalQuery" >>$output_file 2>&1`
if [ $? -eq 1 ]
then
   echo "Error occured while running query :: $query  " >>$output_file 
   exit 1
fi

#echo "Query :: $query " >>$output_file
echo "Successfully Completed :: $2 " >>$output_file
}

function getFormatForFloat()
{
	charFormat='9999999999999999'
	i=1
	while [ $i -le $float_format ]
	do
		if [ $i -eq 1 ]
		then 
			charFormat=$charFormat'.9'
		else
			charFormat=$charFormat'9'
		fi
		i=`expr $i + 1 `
	done
}

function executeUDFs()
{
	getFormatForFloat
	float_format_udf="CREATE OR REPLACE FUNCTION Utility.to_float_format(value number) RETURN VARCHAR2
AS BEGIN
    return (CASE WHEN ( value != 0 ) THEN (replace(to_char(value,'$charFormat'), ' .','0.')) END);
END ;"
	executeCmdInVerticaDB "$float_format_udf" "Float Format UDF" false
}

function createTrendSummaryTablesIfRequired()
{
	count=`$vsql_cmd -At -h "$vertica_ip" -d "$databasename" -U "$vertica_username" -w "$vertica_password" -c "select count(*) from tables where table_name like '%daily';"`
	if [ $count -eq 0 ]
	then
		`$vsql_cmd -At -h "$vertica_ip" -d "$databasename" -U "$vertica_username" -w "$vertica_password" -f "$RANGERHOME/bin/nikira-DDL-VERTICA-UDF-and-Tables.sql" >>$output_file 2>&1`
	fi
	count=`$vsql_cmd -At -h "$vertica_ip" -d "$databasename" -U "$vertica_username" -w "$vertica_password" -c "select count(*) from tables where table_name like '%daily';"`
	if [ $count -eq 0 ]
	then
		echo "ERROR while creating Trend summary tables ... Exiting with error status"
		exit 1
	fi
}

function GetStartDayOfWeek()
{
    input_date=$1
    week_day=`date +%u --date="$input_date"`
    if [ $week_day -eq 7 ]
    then
        output=`date '+%s' --date="$input_date" -d "-6 days"`
    else
        delta_days=`expr $week_day - 1`
        epoc_time=`date '+%s' --date="$input_date" `
        output=`expr $epoc_time - \( $delta_days \* 60 \* 60 \* 24 \)`
    fi
}

function truncateDataFromTable()
{                                         
	start_date=$1
	end_date=$2
	table_name=$3
	week_rentention=$4

	epoch_start_date=`date '+%s' --date="$start_date"`
    
	GetStartDayOfWeek $start_date
	startDOW=$output
	GetStartDayOfWeek $end_date
	endDOW=$output
	no_of_weeks=`expr \( \( $endDOW - $startDOW \) / \( 60 \* 60 \* 24 \* 7 \) + 1 \)`
	i=1
	count=0
	while [ $i -le $no_of_weeks ]
	do                          
		week_of_year=`expr \( $epoch_start_date + \( $i \* 60 \* 60 \* 24 \* 7 \) \) `
		weeks_of_year[$i]=`date '+%W' -d @$week_of_year`
		i=`expr $i + 1 `
		count=`expr $count + 1`
	done
	i=1
	while [ $i -le $no_of_weeks ]
	do                          
		week_of_year=`expr \( $epoch_start_date - \( \( $i + $week_rentention \) \* 60 \* 60 \* 24 \* 7 \) \) `
		weeks_of_year[`expr $i + $no_of_weeks`]=`date '+%W' -d @$week_of_year`
		i=`expr $i + 1 `
		count=`expr $count + 1`
	done
	i=1
	week_str=""
	while [ $i -le $count ]
	do                          
		week_str="$week_str ${weeks_of_year[$i]},"
		i=`expr $i + 1 `
	done
	week_str="$week_str 99"
	query="DELETE FROM $table_name where WEEK_OF_YEAR in ($week_str);"
	executeCmdInVerticaDB "$query" "Deleting Old Data from Final Summary Table" true

} 

createTrendSummaryTablesIfRequired
executeUDFs
if [ "$ParentTrendSummaryName" == "nik_gprs_cdr_daily" ]
then

	query='truncate table gprs_ts_temp ;'
	executeCmdInVerticaDB "$query" "Truncating TS Temp Table"  false
	
	query=" insert into gprs_ts_temp "
	query=$query" ( "
	query=$query" select tsr_from_dttm, tsr_to_dttm, c.NETWORK_ID, datediff('ss',cast(MIN_TIME_STAMP as timestamp),CAST(MAX_TIME_STAMP as timestamp)), c.PHONE_NUMBER, SUM_DATA_VOLUME, "
	query=$query"        CHARGING_ID, MIN_TIME_STAMP, MAX_TIME_STAMP, ACCOUNT_NAME, FIRST_NAME, "
	query=$query"        MIDDLE_NAME, LAST_NAME, s.IMSI, PRODUCT_TYPE, SUBSCRIBER_TYPE, c.subscriber_id, status, to_number(to_char(tsr_from_dttm,'iw')) "
	query=$query" from nik_gprs_cdr_daily c, subscriber_v s"
	query=$query" where c.PHONE_NUMBER = s.phone_number and c.subscriber_id=s.subscriber_id "
	query=$query" and s.status in (1,2) "
	query=$query" union all "
	query=$query" select tsr_from_dttm, tsr_to_dttm, NETWORK_ID, SUM_DURATION, PHONE_NUMBER, SUM_DATA_VOLUME, "
	query=$query"        CHARGING_ID, MIN_TIME_STAMP, MAX_TIME_STAMP, ACCOUNT_NAME, FIRST_NAME, "
	query=$query"        MIDDLE_NAME, LAST_NAME, IMSI, PRODUCT_TYPE, SUBSCRIBER_TYPE, subscriber_id, subscriber_status, week_of_year "
	query=$query" from nik_gprs_cdr_summary c"
	query=$query" where week_of_year >= $ParentTSLastFromDTTMWeek and week_of_year<= $ParentTSLastToDTTMWeek "
	query=$query" ) ;"
	executeCmdInVerticaDB "$query" "Inserting into TS Temp Table" true
	
	query="truncate table nik_gprs_cdr_daily;"
	executeCmdInVerticaDB "$query" "Truncating Daily Table" false
	
	truncateDataFromTable "$ParentTSLastFromDTTM" "$ParentTSLastToDTTM" "nik_gprs_cdr_summary" "$ReportWeekRetention"
	
	query=' insert into nik_gprs_cdr_summary '
	query=$query' ( select '
	query=$query" GetStartDayOfWeek(tsr_from_dttm), GetStartDayOfWeek(tsr_from_dttm)+7-1/86400, network_id,
	datediff('ss',cast(MIN_TIME_STAMP as timestamp),CAST(MAX_TIME_STAMP as timestamp)), phone_number, sum(sum_data_volume), charging_id, min(min_time_stamp), max(max_time_stamp), account_name, first_name, middle_name, last_name, imsi, product_type, subscriber_type, subscriber_id, subscriber_status, week_of_year "
	query=$query' from gprs_ts_temp ' 
	query=$query' group by (tsr_from_dttm), (tsr_from_dttm),  network_id, phone_number, charging_id, account_name,
	first_name, middle_name, last_name, imsi, product_type, subscriber_type, subscriber_id, subscriber_status, week_of_year,
	max_time_stamp, min_time_stamp'
	query=$query' ); '
	executeCmdInVerticaDB "$query" "Inserting into final summary table" true

elif [ "$ParentTrendSummaryName" == "nik_ipdr_daily" ]
then

	query='truncate table ipdr_ts_temp;'
	executeCmdInVerticaDB "$query" "Truncating TS Temp Table" false
    
 	query=" insert into ipdr_ts_temp ";
	query=$query" ( ";
	query=$query" select tsr_from_dttm, tsr_to_dttm, c.NETWORK_ID, c.PHONE_NUMBER, MIN_TIME_STAMP, ";
	query=$query" 	   MAX_TIME_STAMP,  datediff('ss',CAST(MIN_TIME_STAMP as timestamp),CAST(MAX_TIME_STAMP as timestamp)) , SUM_UPLOAD_DATA_VOLUME, SUM_DOWNLOAD_DATA_VOLUME, ";
	query=$query"        IPDR_TYPE, SUM_VAL, USER_NAME, SESSION_ID, ACCOUNT_NAME, FIRST_NAME, ";
	query=$query"        MIDDLE_NAME, LAST_NAME, s.IMSI, PRODUCT_TYPE, SUBSCRIBER_TYPE, c.subscriber_id, status, to_number(to_char(tsr_from_dttm,'iw')) ";
	query=$query" from nik_ipdr_daily c, subscriber_v s";
	query=$query" where c.PHONE_NUMBER = s.phone_number and c.subscriber_id=s.subscriber_id ";
	query=$query" and s.status in (1,2) ";
	query=$query" union all ";
	query=$query" select tsr_from_dttm, tsr_to_dttm, NETWORK_ID, PHONE_NUMBER, MIN_TIME_STAMP, ";
	query=$query"        MAX_TIME_STAMP, SUM_DURATION, SUM_UPLOAD_DATA_VOLUME, SUM_DOWNLOAD_DATA_VOLUME, ";
	query=$query"        IPDR_TYPE, SUM_VAL, USER_NAME, SESSION_ID, ACCOUNT_NAME, FIRST_NAME, ";
	query=$query"        MIDDLE_NAME, LAST_NAME, IMSI, PRODUCT_TYPE, SUBSCRIBER_TYPE, subscriber_id, subscriber_status, week_of_year ";
	query=$query" from nik_ipdr_summary c";
	query=$query" where week_of_year >= $ParentTSLastFromDTTMWeek and week_of_year<= $ParentTSLastToDTTMWeek";
	query=$query" );";    
	executeCmdInVerticaDB "$query" "Inserting into TS Temp Table" true
	
	query="truncate table nik_ipdr_daily;"
	executeCmdInVerticaDB "$query" "Truncating Daily Table" false
	
	truncateDataFromTable "$ParentTSLastFromDTTM" "$ParentTSLastToDTTM" "nik_ipdr_summary" "$ReportWeekRetention"
	
 	query=' insert into nik_ipdr_summary ';
	query=$query' ( select ';
	query=$query" GetStartDayOfWeek(tsr_from_dttm), GetStartDayOfWeek(tsr_from_dttm)+7-1/86400, network_id, phone_number, min(min_time_stamp), max(max_time_stamp), datediff('ss',CAST(MIN_TIME_STAMP as timestamp),CAST(MAX_TIME_STAMP as timestamp)), sum(sum_upload_data_volume), sum(sum_download_data_volume), ipdr_type, sum(sum_val), user_name, session_id, account_name, first_name, middle_name, last_name, imsi, product_type, subscriber_type, subscriber_id, subscriber_status, week_of_year ";
	query=$query' from ipdr_ts_temp ' ;
	query=$query' group by GetStartDayOfWeek(tsr_from_dttm), GetStartDayOfWeek(tsr_from_dttm)+7-1/86400,  network_id,
	phone_number, ipdr_type, user_name, session_id, account_name, first_name, middle_name, last_name, imsi, product_type,
	subscriber_type, subscriber_id, subscriber_status, week_of_year, min_time_stamp, max_time_stamp';
	query=$query' );'; 
	executeCmdInVerticaDB "$query" "Inserting into final summary table" true

elif [ "$ParentTrendSummaryName" == "nik_recharge_log_daily" ]
then

	query='truncate table recharge_log_ts_temp;'
	executeCmdInVerticaDB "$query" "Truncating TS Temp Table" false

 	query=" insert into recharge_log_ts_temp ";
	query=$query" ( ";
	query=$query" select tsr_from_dttm, tsr_to_dttm, c.NETWORK_ID, c.PHONE_NUMBER, ";
	query=$query"        account_name, total_value, recharge_count , c.subscriber_id, status, to_number(to_char(tsr_from_dttm,'iw')) ";
	query=$query" from nik_recharge_log_daily c, subscriber_v s";
	query=$query" where c.PHONE_NUMBER = s.phone_number  and c.subscriber_id=s.subscriber_id ";
	query=$query" and s.status in (1,2) ";
	query=$query" union all ";
	query=$query" select tsr_from_dttm, tsr_to_dttm, NETWORK_ID, PHONE_NUMBER, ";
	query=$query"        account_name, total_value, recharge_count, subscriber_id, subscriber_status, week_of_year ";
	query=$query" from nik_recharge_log_summary c";
	query=$query" where week_of_year >= $ParentTSLastFromDTTMWeek and week_of_year<= $ParentTSLastToDTTMWeek ";
	query=$query" ); "; 
	executeCmdInVerticaDB "$query" "Inserting into TS Temp Table" true
	
	query="truncate table nik_recharge_log_daily;"
	executeCmdInVerticaDB "$query" "Truncating Daily Table" false

	truncateDataFromTable "$ParentTSLastFromDTTM" "$ParentTSLastToDTTM" "nik_recharge_log_summary" "$ReportWeekRetention"
	
 	query=' insert into nik_recharge_log_summary ';
	query=$query' ( select ';
	query=$query' GetStartDayOfWeek(tsr_from_dttm), GetStartDayOfWeek(tsr_from_dttm)+7-1/86400, network_id, phone_number, account_name, sum(total_value), sum(recharge_count), subscriber_id, subscriber_status, week_of_year ';
	query=$query' from recharge_log_ts_temp ' ;
	query=$query' group by GetStartDayOfWeek(tsr_from_dttm), GetStartDayOfWeek(tsr_from_dttm)+7-1/86400,  network_id, phone_number, account_name, subscriber_id, subscriber_status, week_of_year';
	query=$query' ); ';    
	executeCmdInVerticaDB "$query" "Inserting into final summary table" true

elif [ "$ParentTrendSummaryName" == "nik_cdr_per_call_daily" ]
then

	query='truncate table cdr_percall_ts_temp;'
	executeCmdInVerticaDB "$query" "Truncating TS Temp Table" false

 	query=" insert into cdr_percall_ts_temp ";
	query=$query" ( ";
	query=$query" ( ";
	query=$query" select tsr_from_dttm, tsr_to_dttm, band_id, c.NETWORK_ID, c.PHONE_NUMBER,record_type,  ";
	query=$query" cdr_type, decode(cdr_type, 5, 200+CALL_TYPE, 6, 300+CALL_TYPE, 9, 400+CALL_TYPE, CALL_TYPE) as CALL_TYPE, sum_duration,  ";
	query=$query" SUM_VAL, count_xdr_id, account_name, first_name, middle_name, last_name,imsi, product_type, SUBSCRIBER_TYPE, c.subscriber_id, status, to_number(to_char(tsr_from_dttm,'iw')), vpmn ";
	query=$query" from nik_cdr_per_call_daily c, subscriber_v s";
	query=$query" where c.PHONE_NUMBER = s.phone_number and c.subscriber_id=s.subscriber_id ";
	query=$query" and s.status in (1,2) ";
	query=$query" union all ";
	query=$query" select tsr_from_dttm, tsr_to_dttm, band_id, c.NETWORK_ID, c.PHONE_NUMBER,record_type,  ";
	query=$query" cdr_type, decode(cdr_type, 5, 200+CALL_TYPE, 6, 300+CALL_TYPE, 9, 400+CALL_TYPE, CALL_TYPE) as CALL_TYPE, sum_duration, ";
	query=$query" SUM_VAL, ";
	query=$query" count_xdr_id, 'Default Account', 'Default Subscriber', '', '', 'Not Available', 0, 0, c.subscriber_id, 1, to_number(to_char(tsr_from_dttm,'iw')), vpmn ";
	query=$query" from nik_cdr_per_call_daily c";
	query=$query" where c.subscriber_id=2 and c.CDR_TYPE != 6";
	query=$query" union all ";
	query=$query" select tsr_from_dttm, tsr_to_dttm, band_id, c.NETWORK_ID, c.PHONE_NUMBER,record_type,  ";
	query=$query" cdr_type, decode(cdr_type, 5, 200+CALL_TYPE, 6, 300+CALL_TYPE, 9, 400+CALL_TYPE, CALL_TYPE) as CALL_TYPE, sum_duration, ";
	query=$query" SUM_VAL * s.sdr_value, ";
	query=$query" count_xdr_id, 'Default Account', 'Default Subscriber', '', '', 'Not Available', 0, 0, c.subscriber_id, 1, to_number(to_char(tsr_from_dttm,'iw')), vpmn ";
	query=$query" from nik_cdr_per_call_daily c, sdr_rates s";
	query=$query" where c.subscriber_id=2 and c.CDR_TYPE = 6";
	query=$query" and c.tsr_from_dttm > s.start_date";
	query=$query" and s.start_date = (select max(s1.start_date) from sdr_rates s1, nik_cdr_per_call_daily c1 where c1.tsr_from_dttm > s1.start_date and c1.subscriber_id=2 and c1.CDR_TYPE = 6)";
	query=$query" ) ";
	query=$query" union all ";
	query=$query" select tsr_from_dttm, tsr_to_dttm, band_id, NETWORK_ID, PHONE_NUMBER, ";
	query=$query" 	   record_type, cdr_type, call_type, sum_duration, sum_val, count_xdr_id, account_name, first_name, middle_name, last_name, ";
	query=$query"        imsi, product_type, SUBSCRIBER_TYPE , subscriber_id, subscriber_status, week_of_year, vpmn ";
	query=$query" from nik_cdr_per_call_summary c";
	query=$query" where week_of_year >= $ParentTSLastFromDTTMWeek and week_of_year<= $ParentTSLastToDTTMWeek ";
	query=$query" );";   
	executeCmdInVerticaDB "$query" "Inserting into TS Temp Table" true

	query="truncate table nik_cdr_per_call_daily;"
	executeCmdInVerticaDB "$query" "Truncating Daily Table" false

	truncateDataFromTable "$ParentTSLastFromDTTM" "$ParentTSLastToDTTM" "nik_cdr_per_call_summary" "$ReportWeekRetention"
	
    query=' insert into nik_cdr_per_call_summary ';
	query=$query' ( select ';
	query=$query' GetStartDayOfWeek(tsr_from_dttm), GetStartDayOfWeek(tsr_from_dttm)+7-1/86400, band_id, network_id, phone_number, record_type, cdr_type, call_type, sum(sum_duration), sum(sum_val), sum(count_xdr_id), account_name, first_name, middle_name, last_name, imsi, product_type, subscriber_type, subscriber_id, subscriber_status, week_of_year, vpmn ';
	query=$query' from cdr_percall_ts_temp ' ;
	query=$query' group by GetStartDayOfWeek(tsr_from_dttm), GetStartDayOfWeek(tsr_from_dttm)+7-1/86400,  band_id, network_id, phone_number, record_type, cdr_type, call_type, account_name, first_name, middle_name, last_name, imsi, product_type, subscriber_type, subscriber_id, subscriber_status, week_of_year,vpmn ';
	query=$query' );'; 
	executeCmdInVerticaDB "$query" "Inserting into final summary table" true

elif [ "$ParentTrendSummaryName" == "nik_cdr_offpeak_daily" ]
then

	query='truncate table cdr_offpeak_ts_temp;'
	executeCmdInVerticaDB "$query" "Truncating TS Temp Table" false

    query=$query" insert into cdr_offpeak_ts_temp ";
	query=$query" ( ";
	query=$query" select tsr_from_dttm, tsr_to_dttm, band_id, c.NETWORK_ID, c.PHONE_NUMBER, ";
	query=$query" record_type, cdr_type, decode(cdr_type, 5, 200+CALL_TYPE, 6, 300+CALL_TYPE, 9, 400+CALL_TYPE, CALL_TYPE) as CALL_TYPE,  ";
	query=$query" sum_duration, SUM_VAL, count_xdr_id, account_name, first_name, middle_name, last_name,imsi, product_type, SUBSCRIBER_TYPE,  ";
	query=$query" c.subscriber_id, status, to_number(to_char(tsr_from_dttm,'iw')) ";
	query=$query" from nik_cdr_offpeak_daily c, subscriber_v s";
	query=$query" where c.PHONE_NUMBER = s.phone_number and c.subscriber_id=s.subscriber_id ";
	query=$query" and s.status in (1,2) ";
	query=$query" union all ";
	query=$query" select tsr_from_dttm, tsr_to_dttm, band_id, NETWORK_ID, PHONE_NUMBER, ";
	query=$query"        record_type, cdr_type, call_type, sum_duration, sum_val, count_xdr_id, account_name, first_name, middle_name, last_name, ";
	query=$query"        imsi, product_type, SUBSCRIBER_TYPE , subscriber_id, subscriber_status, week_of_year ";
	query=$query" from nik_cdr_offpeak_summary c";
	query=$query" where week_of_year >= $ParentTSLastFromDTTMWeek and week_of_year<= $ParentTSLastToDTTMWeek ";
	query=$query" );"; 
	executeCmdInVerticaDB "$query" "Inserting into TS Temp Table" true

	query="truncate table nik_cdr_offpeak_daily;"
	executeCmdInVerticaDB "$query" "Truncating Daily Table" false

	truncateDataFromTable "$ParentTSLastFromDTTM" "$ParentTSLastToDTTM" "nik_cdr_offpeak_summary" "$ReportWeekRetention"
	
    query=' insert into nik_cdr_offpeak_summary ';
	query=$query' ( select ';
	query=$query' GetStartDayOfWeek(tsr_from_dttm), GetStartDayOfWeek(tsr_from_dttm)+7-1/86400, band_id, network_id, phone_number, record_type, cdr_type, call_type, sum(sum_duration), sum(sum_val), sum(count_xdr_id), account_name, first_name, middle_name, last_name, imsi, product_type, subscriber_type, subscriber_id, subscriber_status, week_of_year ';
	query=$query' from cdr_offpeak_ts_temp ' ;
	query=$query' group by GetStartDayOfWeek(tsr_from_dttm), GetStartDayOfWeek(tsr_from_dttm)+7-1/86400,  band_id, network_id, phone_number, record_type, cdr_type, call_type, account_name, first_name, middle_name, last_name, imsi, product_type, subscriber_type, subscriber_id, subscriber_status, week_of_year';
	query=$query' );'; 
	executeCmdInVerticaDB "$query" "Inserting into final summary table" true

else
	echo "WEEKLY TREND SUMMARY NOT TRIGGERED SINCE ITS PARENT TREND SUMMARY WAS NOT A DAILY BASED TREND SUMMARY."
fi 
