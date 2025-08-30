#!/bin/bash
TaskId=$1                                                                                                                        

ParentTaskId=`sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID <<EOF
	SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF
	select TSK_PARENT_TSK_ID from task where TSK_ID = $TaskId;
	EXIT
EOF`

echo "PARENT TASK ID : ("$ParentTaskId")"

ParentTrendSummaryName=`sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID <<EOF
    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF
	select STST_NAME from TREND_SUMMARY_TABLE_CFG where TSK_ID = $ParentTaskId;
    EXIT
EOF`

ParentTSLastFromDTTMWeek=`sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID <<EOF  
    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF                                                                                     
    select to_char(STCR_FROM_DTTM,'iw') from TREND_SUMMARY_TBL_CFG_RUN where tsk_id=$ParentTaskId and STCR_ID = (select max(stcr_id) from TREND_SUMMARY_TBL_CFG_RUN where tsk_id=$ParentTaskId);                                                                                                                          
    EXIT                                                                                                                                                     
EOF`

ParentTSLastToDTTMWeek=`sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID <<EOF                                                                                 
    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF                                                                                     
    select to_char(STCR_TO_DTTM,'iw') from TREND_SUMMARY_TBL_CFG_RUN where tsk_id=$ParentTaskId and STCR_ID = (select max(stcr_id) from TREND_SUMMARY_TBL_CFG_RUN where tsk_id=$ParentTaskId);                                                                                                                          
    EXIT                                                                                                                                                     
EOF`

ParentTSLastFromDTTM=`sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID <<EOF                                                                                 
    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF                                                                                     
    select to_char(STCR_FROM_DTTM,'yyyy/mm/dd') from TREND_SUMMARY_TBL_CFG_RUN where tsk_id=$ParentTaskId and STCR_ID = (select max(stcr_id) from TREND_SUMMARY_TBL_CFG_RUN where tsk_id=$ParentTaskId); 
    EXIT                                                                                                                                                     
EOF`

ParentTSLastToDTTM=`sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID <<EOF                                                                                     
    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF                                                                                     
    select to_char(STCR_FROM_DTTM,'yyyy/mm/dd') from TREND_SUMMARY_TBL_CFG_RUN where tsk_id=$ParentTaskId and STCR_ID = (select max(stcr_id) from TREND_SUMMARY_TBL_CFG_RUN where tsk_id=$ParentTaskId);                                                                                                                  
    EXIT                                                                                                                                                     
EOF`

ReportWeekRetention=`sqlplus -s $DB_USER/$DB_PASSWORD@$ORACLE_SID <<EOF
    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF                                                                                     
	select value from configurations where config_key='REPORTS_WEEK_RETENTION'; 
    EXIT                                                                                                                                                     
EOF`

float_precision=`sqlplus -s $DB_USER/$DB_PASSWORD@$ORACLE_SID <<EOF
    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF                                                                                     
	select value from configurations where config_key='DEFAULT_FLOAT_PRECISION'; 
    EXIT                                                                                                                                                     
EOF`

echo "PARENT TASK NAME : ("$ParentTrendSummaryName")"


if [ "$ParentTrendSummaryName" == "nik_gprs_cdr_daily" ]
then                                                            

sqlplus $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID << EOF
declare
        query        varchar2(4000) := '' ;
begin
		begin
        execute immediate 'truncate table gprs_ts_temp ';
		query := query || ' insert into /*+ append nologging */ gprs_ts_temp ';
		query := query || ' ( ';
		query := query || ' select /*+ parallel (c,4) parallel (s,4) */ tsr_from_dttm, tsr_to_dttm, c.NETWORK_ID, cast(CAST(MAX_TIME_STAMP as date) - CAST(MIN_TIME_STAMP as date) as number) * 86400, c.PHONE_NUMBER, SUM_DATA_VOLUME, ';
		query := query || '        CHARGING_ID, MIN_TIME_STAMP, MAX_TIME_STAMP, ACCOUNT_NAME, FIRST_NAME, ';
		query := query || '        MIDDLE_NAME, LAST_NAME, s.IMSI, PRODUCT_TYPE, SUBSCRIBER_TYPE, c.subscriber_id, status, to_number(to_char(tsr_from_dttm,''iw'')) ';
		query := query || ' from nik_gprs_cdr_daily c, nik_subscriber_v s';
		query := query || ' where c.PHONE_NUMBER = s.phone_number and c.subscriber_id=s.subscriber_id ';
		query := query || ' and s.status in (1,2) ';
		query := query || ' union all ';
		query := query || ' select /*+ parallel (c,4)  */ tsr_from_dttm, tsr_to_dttm, NETWORK_ID, SUM_DURATION, PHONE_NUMBER, SUM_DATA_VOLUME, ';
		query := query || '        CHARGING_ID, MIN_TIME_STAMP, MAX_TIME_STAMP, ACCOUNT_NAME, FIRST_NAME, ';
		query := query || '        MIDDLE_NAME, LAST_NAME, IMSI, PRODUCT_TYPE, SUBSCRIBER_TYPE, subscriber_id, subscriber_status, week_of_year ';
		query := query || ' from nik_gprs_cdr_summary c';
		query := query || ' where week_of_year >= $ParentTSLastFromDTTMWeek and week_of_year<= $ParentTSLastToDTTMWeek ';
		query := query || ' ) ';

		execute immediate query ;
        commit;
		exception
	    when others then
			raise_application_error(-20001, 'Join with subscriber and insertion into gprs_ts_temp table failed.');
	        return ;
        end ;
		execute immediate 'truncate table nik_gprs_cdr_daily';
        TrendSummary.truncateDataFromTable(to_date('$ParentTSLastFromDTTM','yyyy/mm/dd'),to_date('$ParentTSLastToDTTM','yyyy/mm/dd'), 'nik_gprs_cdr_summary');

		begin
			TrendSummary.GPRS_CDR_SUMMARY ;
		exception
        when others then
			raise_application_error(-20002, 'GENERATION OF GPRS CDR SUMMARY FAILED.');
            return ;
        end ;
end ;
/

EOF

elif [ "$ParentTrendSummaryName" == "nik_ipdr_daily" ]
then
sqlplus $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID << EOF

declare
        query        varchar2(4000) := '' ;
begin
		begin
        execute immediate 'truncate table ipdr_ts_temp ';
		query := query || ' insert into /*+ append nologging */ ipdr_ts_temp ';
		query := query || ' ( ';
		query := query || ' select /*+ parallel (c,4) parallel (s,4) */ tsr_from_dttm, tsr_to_dttm, c.NETWORK_ID, c.PHONE_NUMBER, MIN_TIME_STAMP, ';
		query := query || ' 	   MAX_TIME_STAMP,  cast(CAST(MAX_TIME_STAMP as date) - CAST(MIN_TIME_STAMP as date) as number) * 86400 , SUM_UPLOAD_DATA_VOLUME, SUM_DOWNLOAD_DATA_VOLUME, ';
		query := query || '        IPDR_TYPE, SUM_VAL, USER_NAME, SESSION_ID, ACCOUNT_NAME, FIRST_NAME, ';
		query := query || '        MIDDLE_NAME, LAST_NAME, s.IMSI, PRODUCT_TYPE, SUBSCRIBER_TYPE, c.subscriber_id, status, to_number(to_char(tsr_from_dttm,''iw'')) ';
		query := query || ' from nik_ipdr_daily c, nik_subscriber_v s';
		query := query || ' where c.PHONE_NUMBER = s.phone_number and c.subscriber_id=s.subscriber_id ';
		query := query || ' and s.status in (1,2) ';
		query := query || ' union all ';
		query := query || ' select /*+ parallel (c,4)  */ tsr_from_dttm, tsr_to_dttm, NETWORK_ID, PHONE_NUMBER, MIN_TIME_STAMP, ';
		query := query || '        MAX_TIME_STAMP, SUM_DURATION, SUM_UPLOAD_DATA_VOLUME, SUM_DOWNLOAD_DATA_VOLUME, ';
		query := query || '        IPDR_TYPE, SUM_VAL, USER_NAME, SESSION_ID, ACCOUNT_NAME, FIRST_NAME, ';
		query := query || '        MIDDLE_NAME, LAST_NAME, IMSI, PRODUCT_TYPE, SUBSCRIBER_TYPE, subscriber_id, subscriber_status, week_of_year ';
		query := query || ' from nik_ipdr_summary c';
		query := query || ' where week_of_year >= $ParentTSLastFromDTTMWeek and week_of_year<= $ParentTSLastToDTTMWeek';
		query := query || ' ) ';
		execute immediate query ;
        commit;
		exception
        when others then
			raise_application_error(-20001, 'Join with subscriber and insertion into ipdr_ts_temp table failed.');
            return ;
        end ;

		execute immediate 'truncate table nik_ipdr_daily';

		TrendSummary.truncateDataFromTable(to_date('$ParentTSLastFromDTTM','yyyy/mm/dd'),to_date('$ParentTSLastToDTTM','yyyy/mm/dd'), 'nik_ipdr_summary');

		begin
			TrendSummary.IPDR_SUMMARY ;
		exception
        when others then
			raise_application_error(-20002, 'GENERATION OF IPDR SUMMARY FAILED.');
            return ;
        end ;
end ;
/
EOF

elif [ "$ParentTrendSummaryName" == "nik_recharge_log_daily" ]
then
sqlplus $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID << EOF

declare
        query        varchar2(4000) := '' ;
begin
		begin
        execute immediate 'truncate table recharge_log_ts_temp ';
		query := query || ' insert into /*+ append nologging */ recharge_log_ts_temp ';
		query := query || ' ( ';
		query := query || ' select /*+ parallel (c,4) parallel (s,4) */ tsr_from_dttm, tsr_to_dttm, c.NETWORK_ID, c.PHONE_NUMBER, ';
		query := query || '        account_name, total_value, recharge_count , c.subscriber_id, status, to_number(to_char(tsr_from_dttm,''iw'')) ';
		query := query || ' from nik_recharge_log_daily c, nik_subscriber_v s';
		query := query || ' where c.PHONE_NUMBER = s.phone_number  and c.subscriber_id=s.subscriber_id ';
		query := query || ' and s.status in (1,2) ';
		query := query || ' union all ';
		query := query || ' select /*+ parallel (c,4)  */ tsr_from_dttm, tsr_to_dttm, NETWORK_ID, PHONE_NUMBER, ';
		query := query || '        account_name, total_value, recharge_count, subscriber_id, subscriber_status, week_of_year ';
		query := query || ' from nik_recharge_log_summary c';
		query := query || ' where week_of_year >= $ParentTSLastFromDTTMWeek and week_of_year<= $ParentTSLastToDTTMWeek ';
		query := query || ' ) ';
		execute immediate query ;
        commit;
		exception
        when others then
			raise_application_error(-20001, 'Join with subscriber and insertion into recharge_log_ts_temp table failed.');
            return ;
        end ;

		execute immediate 'truncate table nik_recharge_log_daily';

		TrendSummary.truncateDataFromTable(to_date('$ParentTSLastFromDTTM','yyyy/mm/dd'),to_date('$ParentTSLastToDTTM','yyyy/mm/dd'), 'nik_recharge_log_summary');

		begin
			TrendSummary.RECHARGE_LOG_SUMMARY ;
		exception
        when others then
			raise_application_error(-20002, 'GENERATION OF RECHARGE LOG SUMMARY FAILED.');
            return ;
        end ;
end ;
/
EOF

elif [ "$ParentTrendSummaryName" == "nik_cdr_per_call_daily" ]
then
sqlplus $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID << EOF

declare
        query        varchar2(4000) := '' ;
begin
		begin
        execute immediate 'truncate table cdr_percall_ts_temp ';
		query := query || ' insert into /*+ append nologging */ cdr_percall_ts_temp ';
		query := query || ' ( ';
		query := query || ' ( ';
		query := query || ' select /*+ parallel (c,4) parallel (s,4) */ tsr_from_dttm, tsr_to_dttm, band_id, c.NETWORK_ID, c.PHONE_NUMBER,record_type,  ';
		query := query || ' cdr_type, decode(cdr_type, 5, 200+CALL_TYPE, 6, 300+CALL_TYPE, 9, 400+CALL_TYPE, CALL_TYPE) as CALL_TYPE, sum_duration,  ';
		query := query || ' SUM_VAL, count_xdr_id, account_name, first_name, middle_name, last_name,imsi, product_type, SUBSCRIBER_TYPE, c.subscriber_id, status, to_number(to_char(tsr_from_dttm,''iw'')), vpmn ';
		query := query || ' from nik_cdr_per_call_daily c, nik_subscriber_v s';
		query := query || ' where c.PHONE_NUMBER = s.phone_number and c.subscriber_id=s.subscriber_id ';
		query := query || ' and s.status in (1,2) ';
		query := query || ' union all ';
		query := query || ' select /*+ parallel (c,4)  */ tsr_from_dttm, tsr_to_dttm, band_id, c.NETWORK_ID, c.PHONE_NUMBER,record_type,  ';
		query := query || ' cdr_type, decode(cdr_type, 5, 200+CALL_TYPE, 6, 300+CALL_TYPE, 9, 400+CALL_TYPE, CALL_TYPE) as CALL_TYPE, sum_duration, ';
		query := query || ' DECODE(CDR_TYPE, 6,SUM_VAL*SDRValue(tsr_from_dttm),SUM_VAL), ';
		query := query || ' count_xdr_id, ''Default Account'', ''Default Subscriber'', '''', '''', ''Not Available'', 0, 0, c.subscriber_id, 1, to_number(to_char(tsr_from_dttm,''iw'')), vpmn ';
		query := query || ' from nik_cdr_per_call_daily c';
		query := query || ' where c.subscriber_id=2 ';
		query := query || ' ) ';
		query := query || ' union all ';
		query := query || ' select /*+ parallel (c,4)  */ tsr_from_dttm, tsr_to_dttm, band_id, NETWORK_ID, PHONE_NUMBER, ';
		query := query || ' 	   record_type, cdr_type, call_type, sum_duration, sum_val, count_xdr_id, account_name, first_name, middle_name, last_name, ';
		query := query || '        imsi, product_type, SUBSCRIBER_TYPE , subscriber_id, subscriber_status, week_of_year, vpmn ';
		query := query || ' from nik_cdr_per_call_summary c';
		query := query || ' where week_of_year >= $ParentTSLastFromDTTMWeek and week_of_year<= $ParentTSLastToDTTMWeek ';
		query := query || ' ) ';  
		execute immediate query ;
        commit;
		exception
        when others then 
			raise_application_error(-20001, 'Join with subscriber and insertion into cdr_percall_ts_temp table failed.');
            return ;
        end ;

		execute immediate 'truncate table nik_cdr_per_call_daily';

		TrendSummary.truncateDataFromTable(to_date('$ParentTSLastFromDTTM','yyyy/mm/dd'),to_date('$ParentTSLastToDTTM','yyyy/mm/dd'), 'nik_cdr_per_call_summary');

		begin
			TrendSummary.CDR_PER_CALL_SUMMARY ;
		exception
        when others then
			raise_application_error(-20002, 'GENERATION OF PER CALL SUMMARY FAILED.');
            return ;
        end ;
end ;
/
EOF

elif [ "$ParentTrendSummaryName" == "nik_cdr_offpeak_daily" ]
then
sqlplus $SPARK_DB_USER/$SPARK_DB_PASSWORD@$SPARK_ORACLE_SID << EOF

declare
        query        varchar2(4000) := '' ;
begin
		begin
        execute immediate 'truncate table cdr_offpeak_ts_temp ';
		query := query || ' insert into /*+ append nologging */ cdr_offpeak_ts_temp ';
		query := query || ' ( ';
		query := query || ' select /*+ parallel (c,4) parallel (s,4) */ tsr_from_dttm, tsr_to_dttm, band_id, c.NETWORK_ID, c.PHONE_NUMBER, ';
		query := query || ' record_type, cdr_type, decode(cdr_type, 5, 200+CALL_TYPE, 6, 300+CALL_TYPE, 9, 400+CALL_TYPE, CALL_TYPE) as CALL_TYPE,  ';
		query := query || ' sum_duration, SUM_VAL, count_xdr_id, account_name, first_name, middle_name, last_name,imsi, product_type, SUBSCRIBER_TYPE,  ';
		query := query || ' c.subscriber_id, status, to_number(to_char(tsr_from_dttm,''iw'')) ';
		query := query || ' from nik_cdr_offpeak_daily c, nik_subscriber_v s';
		query := query || ' where c.PHONE_NUMBER = s.phone_number and c.subscriber_id=s.subscriber_id ';
		query := query || ' and s.status in (1,2) ';
		query := query || ' union all ';
		query := query || ' select /*+ parallel (c,4)  */ tsr_from_dttm, tsr_to_dttm, band_id, NETWORK_ID, PHONE_NUMBER, ';
		query := query || '        record_type, cdr_type, call_type, sum_duration, sum_val, count_xdr_id, account_name, first_name, middle_name, last_name, ';
		query := query || '        imsi, product_type, SUBSCRIBER_TYPE , subscriber_id, subscriber_status, week_of_year ';
		query := query || ' from nik_cdr_offpeak_summary c';
		query := query || ' where week_of_year >= $ParentTSLastFromDTTMWeek and week_of_year<= $ParentTSLastToDTTMWeek ';
		query := query || ' ) ';
		execute immediate query ;
        commit;
		exception
        when others then
			raise_application_error(-20001, 'Join with subscriber and insertion into cdr_offpeak_ts_temp table failed.');
            return ;
        end ;

        execute immediate 'truncate table nik_cdr_offpeak_daily';

		TrendSummary.truncateDataFromTable(to_date('$ParentTSLastFromDTTM','yyyy/mm/dd'),to_date('$ParentTSLastToDTTM','yyyy/mm/dd'), 'nik_cdr_offpeak_summary');

		begin
			TrendSummary.CDR_OFFPEAK_SUMMARY ;
		exception
        when others then
			raise_application_error(-20002, 'GENERATION OF OFFPEAK SUMMARY FAILED.');
            return ;
        end ;

end ;
/
EOF
else
	echo "WEEKLY TREND SUMMARY NOT TRIGGERED SINCE ITS PARENT TREND SUMMARY WAS NOT A DAILY BASED TREND SUMMARY."
fi
