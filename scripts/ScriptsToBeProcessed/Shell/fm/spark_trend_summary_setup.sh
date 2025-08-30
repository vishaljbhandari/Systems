username=$1
password=$2

sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD << EOF
 drop table nik_cdr_summary ;
 drop table nik_gprs_cdr_summary ;
 drop table nik_ipdr_summary ;
 drop table nik_cdr_per_call_summary ;
 drop table nik_cdr_offpeak_summary ;
 drop table nik_recharge_log_summary ;
 delete from table_inst where tbd_id = 100004 or tbd_id=100100;
 INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100100, 2, 'pin_reuse_check', 'pin_reuse_check', 'prc', 'N', 1, 1); 
 update next_object_id set noi_current_no=(select max(TIN_ID) from table_inst) where noi_object_name='TableInst';
create table nik_cdr_summary
(
 TSR_ID                                    NUMBER(19)       not null,
 TSR_FROM_DTTM                             TIMESTAMP(6)     not null,
 TSR_TO_DTTM                               TIMESTAMP(6)     not null,
 TSR_MISSING_FL                            CHAR(1)          not null,
 NETWORK_ID                                NUMBER(10)       ,
 PHONE_NUMBER                              VARCHAR2(255)    ,
 RECORD_TYPE                               NUMBER(10)       ,
 CDR_TYPE                                  NUMBER(10)       ,
 CALL_TYPE                                 NUMBER(10)       ,
 SUM_DURATION                              NUMBER(19)       ,
 SUM_VAL                                   NUMBER(19)       ,
 COUNT_XDR_ID                              NUMBER(19)       ,
 ACCOUNT_NAME                              VARCHAR2(255)    ,
 FIRST_NAME                                VARCHAR2(255)    ,
 MIDDLE_NAME                               VARCHAR2(255)    ,
 LAST_NAME                                 VARCHAR2(255)    ,
 IMSI                                      VARCHAR2(255)    ,
 PRODUCT_TYPE                              NUMBER(10)       ,
 SUBSCRIBER_TYPE                           NUMBER(10)       ,
 SUBSCRIBER_ID                             NUMBER(19)	   ,
 SUBSCRIBER_STATUS                         NUMBER(19)       ,
 VPMN									  VARCHAR2(255)
);
                           
create table nik_gprs_cdr_summary
(
 TSR_ID                                     NUMBER(19)		not null,
 TSR_FROM_DTTM                              TIMESTAMP(6)    not null,
 TSR_TO_DTTM                                TIMESTAMP(6)    not null,
 TSR_MISSING_FL                             CHAR(1)         not null,
 NETWORK_ID                                 NUMBER(10)      ,
 SUM_DURATION                               NUMBER(19)      ,
 PHONE_NUMBER                               VARCHAR2(255)   ,
 SUM_DATA_VOLUME                            NUMBER(19)      ,
 CHARGING_ID                                VARCHAR2(255)   ,
 MIN_TIME_STAMP                             TIMESTAMP(6)    ,
 MAX_TIME_STAMP                             TIMESTAMP(6)    ,
 ACCOUNT_NAME                               VARCHAR2(255)   ,
 FIRST_NAME                                 VARCHAR2(255)   ,
 MIDDLE_NAME                                VARCHAR2(255)   ,
 LAST_NAME                                  VARCHAR2(255)   ,
 IMSI                                       VARCHAR2(255)   ,
 PRODUCT_TYPE                               NUMBER(10)      ,
 SUBSCRIBER_TYPE                            NUMBER(10)      ,
 SUBSCRIBER_ID                              NUMBER(19)      ,
 SUBSCRIBER_STATUS                          NUMBER(19)
);

create table nik_ipdr_summary
(
 TSR_ID                                    NUMBER(19)       not null,
 TSR_FROM_DTTM                             TIMESTAMP(6)     not null,
 TSR_TO_DTTM                               TIMESTAMP(6)     not null,
 TSR_MISSING_FL                            CHAR(1)          not null,
 NETWORK_ID                                NUMBER(10)       ,
 PHONE_NUMBER                              VARCHAR2(255)    ,
 MIN_TIME_STAMP                            TIMESTAMP(6)     ,
 MAX_TIME_STAMP                            TIMESTAMP(6)     ,
 SUM_DURATION                              NUMBER(19)       ,
 SUM_UPLOAD_DATA_VOLUME                    NUMBER(19)       ,
 SUM_DOWNLOAD_DATA_VOLUME                  NUMBER(19)       ,
 IPDR_TYPE                                 NUMBER(10)       ,
 SUM_VAL                                   NUMBER(19)       ,
 USER_NAME                                 VARCHAR2(255)    ,
 SESSION_ID                                VARCHAR2(255)    ,
 ACCOUNT_NAME                              VARCHAR2(255)    ,
 FIRST_NAME                                VARCHAR2(255)    ,
 MIDDLE_NAME                               VARCHAR2(255)    ,
 LAST_NAME                                 VARCHAR2(255)    ,
 IMSI                                      VARCHAR2(255)    ,
 PRODUCT_TYPE                              NUMBER(10)       ,
 SUBSCRIBER_TYPE                           NUMBER(10)       ,
 SUBSCRIBER_ID                             NUMBER(19)       ,
 SUBSCRIBER_STATUS                         NUMBER(19)
);                         
create table nik_cdr_per_call_summary
(
TSR_ID                                    NUMBER(19)		not null,
TSR_FROM_DTTM                             TIMESTAMP(6)     not null,
TSR_TO_DTTM                               TIMESTAMP(6)     not null,
TSR_MISSING_FL                            CHAR(1)          not null,
BAND_ID                                   NUMBER(10)       not null,
NETWORK_ID                                NUMBER(10)       not null,
PHONE_NUMBER                              VARCHAR2(255)    not null,
RECORD_TYPE                               NUMBER(10)       not null,
CDR_TYPE                                  NUMBER(10)       not null,
CALL_TYPE                                 NUMBER(10)       not null,
SUM_DURATION                              NUMBER(19)       not null,
SUM_VAL                                   NUMBER(19)       not null,
COUNT_XDR_ID                              NUMBER(19)       not null,
ACCOUNT_NAME                              VARCHAR2(255)    not null,
FIRST_NAME                                VARCHAR2(255)    ,
MIDDLE_NAME                               VARCHAR2(255)    ,
LAST_NAME                                 VARCHAR2(255)    ,
IMSI                                      VARCHAR2(255)    ,
PRODUCT_TYPE                              NUMBER(19)       not null,
SUBSCRIBER_TYPE                           NUMBER(19)       ,
 SUBSCRIBER_ID                             NUMBER(19)       ,
 SUBSCRIBER_STATUS                         NUMBER(19)
);
create table nik_cdr_offpeak_summary
(
 TSR_ID                                     NUMBER(19)       not null,
 TSR_FROM_DTTM                              TIMESTAMP(6)     not null,
 TSR_TO_DTTM                                TIMESTAMP(6)     not null,
 TSR_MISSING_FL                             CHAR(1)          not null,
 BAND_ID                                    NUMBER(10)       not null,
 NETWORK_ID                                 NUMBER(10)       not null,
 PHONE_NUMBER                               VARCHAR2(255)    not null,
 RECORD_TYPE                                NUMBER(10)       not null,
 CDR_TYPE                                   NUMBER(10)       not null,
 CALL_TYPE                                  NUMBER(10)       not null,
 SUM_DURATION                               NUMBER(19)       not null,
 SUM_VAL                                    NUMBER(19)       not null,
 COUNT_XDR_ID                               NUMBER(19)       not null,
 ACCOUNT_NAME                               VARCHAR2(255)    not null,
 FIRST_NAME                                 VARCHAR2(255)    ,
 MIDDLE_NAME                                VARCHAR2(255)    ,
 LAST_NAME                                  VARCHAR2(255)    ,
 IMSI                                       VARCHAR2(255)    not null,
 PRODUCT_TYPE                               NUMBER(19)       not null,
 SUBSCRIBER_TYPE                            NUMBER(19)       ,
 SUBSCRIBER_ID                              NUMBER(19)       ,
 SUBSCRIBER_STATUS                          NUMBER(19)
);

create table nik_recharge_log_summary
(
 TSR_ID                                     NUMBER(19)       not null,
 TSR_FROM_DTTM                              TIMESTAMP(6)     not null,
 TSR_TO_DTTM                                TIMESTAMP(6)     not null,
 TSR_MISSING_FL                             CHAR(1)          not null,
 NETWORK_ID                                 NUMBER(10)       ,
 PHONE_NUMBER                               VARCHAR2(255)    ,
 ACCOUNT_NAME                               VARCHAR2(255)    ,
 TOTAL_VALUE                                NUMBER(19)       ,
 RECHARGE_COUNT                             NUMBER(19)       ,
 SUBSCRIBER_ID                              NUMBER(19)       ,
 SUBSCRIBER_STATUS                          NUMBER(19)
);

create or replace synonym nik_cdr for cdr@$NIKIRA_DB_LINK;
create or replace synonym nik_gprs_cdr for gprs_cdr@$NIKIRA_DB_LINK;
create or replace synonym nik_ipdr for ipdr@$NIKIRA_DB_LINK;
create or replace synonym nik_recharge_log for recharge_log@$NIKIRA_DB_LINK;

EOF

sqlplus -s $username/$password << EOF
create or replace synonym nik_cdr_summary for nik_cdr_summary@$SPARK_DB_LINK;
create or replace synonym nik_gprs_cdr_summary for nik_gprs_cdr_summary@$SPARK_DB_LINK;
create or replace synonym nik_recharge_log_summary for nik_recharge_log_summary@$SPARK_DB_LINK;
create or replace synonym nik_ipdr_summary for nik_ipdr_summary@$SPARK_DB_LINK;
create or replace synonym nik_cdr_per_call_summary for nik_cdr_per_call_summary@$SPARK_DB_LINK;
create or replace synonym nik_cdr_offpeak_summary for nik_cdr_offpeak_summary@$SPARK_DB_LINK;
EOF
