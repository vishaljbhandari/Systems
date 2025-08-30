#!/bin/bash

NikiraDBUser=$1
NikiraDBPassword=$2
NikiraOracleSid=$3
SparkDBUser=$4
SparkDBPasswd=$5
SparkOracleSid=$6
SparkDbLink=$7
FMDBLink=$8



if [ -z $SparkDBUser ]
then
	echo 'SPARK_DB_USER Not Set. Synonym Creation is Not Possible'
	exit 1
fi

if [ -z $SparkDBPasswd ]
then
	echo 'SPARK_DB_PASSWORD Not Set. Synonym Creation is Not Possible'
	exit 1
fi

if [ -z $FMDBLink ]
then
	echo 'NIKIRA_DB_LINK Not Set. Synonym Creation is Not Possible'
	exit 1
fi

if [ -z $SparkDbLink ]
then
	echo 'SPARK_DB_LINK Not Set. Synonym Creation is Not Possible'
	exit 1
fi

if [ -z $SparkOracleSid ]
then
	echo 'SPARK_ORACLE_SID Not Set. Synonym Creation is Not Possible'
	exit 1
fi

if [ -z $NikiraOracleSid ]
then
	echo 'NIKIRA_ORACLE_SID Not Set. Synonym Creation is Not Possible'
	exit 1
fi

SparkDBUser=$SparkDBUser
SparkDBPassword=$SparkDBPasswd
SparkToNikDBLink=$FMDBLink
NikToSparkDBLink=$SparkDbLink

function createTables()
{
MAX_TINID=`sqlplus -s $SparkDBUser/$SparkDBPassword@$SparkOracleSid << EOF | grep -v '^$'
select max(TIN_ID) from TABLE_INST ;
EOF
`
sqlplus -s $SparkDBUser/$SparkDBPassword@$SparkOracleSid<<EOF
set feedback off ;

delete from TABLE_INST where TIN_TABLE_NAME in ('nik_blacklist_profile_options','nik_asn_common_numbers','nik_vpmn_temp','nik_subscriber','nik_lsp_subscriber','nik_auto_suspect_nums_v','nik_temp_id_cdm','nik_rtf_temp_id_cdm','nik_rtf_archive_details','nik_rtf_ar_cdr','nik_rtf_ar_gprs_cdr','nik_rtf_ar_recharge_log','nik_rtf_ar_ipdr','nik_account','nik_subscriber_v','nik_alarms','nik_frequently_called_dest','suspect_values','hotlist_groups_suspect_values','auto_suspect_nums','list_details','hotlist_per_key','temp_osr_xdr_id') ;

INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100002, 2, 'nik_blacklist_profile_options', 'nik_blacklist_profile_options', 'blk', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100003, 2, 'nik_asn_common_numbers', 'nik_asn_common_numbers', 'acn', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100005, 2, 'nik_vpmn_temp', 'nik_vpmn_temp', 'vpt', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100006, 2, 'nik_subscriber', 'nik_subscriber', 'sub', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100007, 2, 'nik_lsp_subscriber', 'nik_lsp_subscriber', 'lsu', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100008, 2, 'nik_auto_suspect_nums_v', 'nik_auto_suspect_nums_v', 'lsu', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100009, 2, 'nik_temp_id_cdm', 'nik_temp_id_cdm', 'lsu', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100010, 2, 'nik_rtf_temp_id_cdm', 'nik_rtf_temp_id_cdm', 'lsu', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100013, 2, 'nik_rtf_archive_details', 'nik_rtf_archive_details', 'radt', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100014, 2, 'nik_rtf_ar_cdr', 'nik_rtf_ar_cdr', 'racdr', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100015, 2, 'nik_rtf_ar_gprs_cdr', 'nik_rtf_ar_gprs_cdr', 'ragprs', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100016, 2, 'nik_rtf_ar_recharge_log', 'nik_rtf_ar_recharge_log', 'rarec', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100017, 2, 'nik_account', 'nik_account', 'acc', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100018, 2, 'nik_subscriber_v', 'nik_subscriber_v', 'sub_v', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100019, 2, 'nik_alarms', 'nik_alarms', 'alarms', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100020, 2, 'nik_frequently_called_dest', 'nik_frequently_called_dest', 'fcdest', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100021, 2, 'nik_rtf_ar_ipdr', 'nik_rtf_ar_ipdr', 'raipdr', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100021, 2, 'suspect_values', 'suspect_values', 'suspval', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100021, 2, 'hotlist_groups_suspect_values', 'hotlist_groups_suspect_values', 'hgrpsuspvals', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100021, 2, 'auto_suspect_nums', 'auto_suspect_nums', 'asuspnum', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100021, 2, 'hotlist_per_key', 'hotlist_per_key', 'hperkey', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100021, 2, 'list_details', 'list_details', 'listdetail', 'N', 1, 1);
INSERT INTO TABLE_INST ("TIN_ID", "TBD_ID", "SCH_ID", "TIN_TABLE_NAME", "TIN_DISPLAY_NAME", "TIN_ALIAS", "TIN_DELETE_FL", "TIN_VERSION_ID", "PTN_ID") VALUES((select max(TIN_ID)+1 from table_inst), 100022, 2, 'temp_osr_xdr_id', 'temp_osr_xdr_id', 'temp_osr_xdr_id', 'N', 1, 1);

update next_object_id set NOI_CURRENT_NO = (select max(tin_id) from table_inst) where NOI_OBJECT_NAME='TableInst';
update property_inst set pri_value ='N' where prd_id in ( select prd_id from property_dfn where prd_key like 'SvrSynchro%') ;

--update service_pack set spk_reference_mappings_file='/com/subex/nikira/database/hibernate/mappings/reference/mappings.hbm.xml', spk_reference_schema_file='/com/subex/nikira/database/metadata/nikira_schema_v1.0.0_sp0.xml' where IVN_ID=(select IVN_ID from INSTALL_VERSION where IVN_NAME='ROC Fraud Management') ;

-- FMS Seed Data 
--@spark_fms_db.sql

EOF
}

function createsynonymsofnikira()
{
tablename=$1
synonymname=nik_$1
	
if [ $# -eq 2 ]
then
	synonymname=$2
fi

sqlplus -s $NikiraDBUser/$NikiraDBPassword@$NikiraOracleSid<< EOF
set feedback off ;

CREATE OR REPLACE SYNONYM $synonymname FOR $SparkDBUser.$tablename@$NikToSparkDBLink ;

EOF

if [ "$SparkOracleSid" = "$NikiraOracleSid" ];then
	AccessUser='public'
else
	AccessUser=$NikiraDBUser
fi

if [ "$SparkOracleSid" = "$NikiraOracleSid" ];then
	sqlplus -s $SparkDBUser/$SparkDBPassword@$SparkOracleSid << EOF
	set feedback off ;

	GRANT ALL on $SparkDBUser.$tablename to $AccessUser ;

EOF
fi
}

if [ $# -eq 0 ]
then
echo "Error: No arguments provided."
exit 1
else

createTables

fi
exit 0
