#!/bin/bash
## DO NOT CHECKIN WITH VALUES
## BEFORE EXECUTING THE ATs, MAKE SURE THAT U HAVE SET THE BELOW COMMENTED TEST ENVIRONMENT VARIABLES
## WITH APPROPRIATE VALUES.

export HOST=10.113.47.160
export USER=user
export DB_SETUP_USER=nikira
export DB_SETUP_PASSWORD=nikira

export DBSETUP_HOME=/home/nikira/8.0db2/DBSetup
export RANGERV6_CLIENT_HOME=/home/nikira/8.0db2/Client/src
export RANGERV6_SERVER_HOME=/home/nikira/8.0db2/Server
export RANGERV6_NOTIFICATIONMANAGER_HOME=/home/nikira/8.0db2/Client/notificationmanager
export WATIR_SERVER_HOME=/home/nikira/Watir
export WINDOW_TIMEOUT=60
export ORAENV_ASK=NO
export IS_MLH_ENABLED=0
export IS_DB2_ENABLED=0
#############################################################################
export IS_CDM_ENABLED=0
export SPARK_DEPLOY_DIR=/home/user/deploy

export CDR_DATASOURCE_INPUTDIR=DataSourceCDRData
export GPRS_DATASOURCE_INPUTDIR=DataSourceGPRSData
export IPDR_DATASOURCE_INPUTDIR=DataSourceIPDRData
export RECHARGELOG_DATASOURCE_INPUTDIR=DataSourceRechargeData
export IS_RATING_ENABLED=1
export CDR_TABLE_NAME_PREFIX=NIK_CDR
export GPRS_TABLE_NAME_PREFIX=NIK_GPRS_CDR
export IPDR_TABLE_NAME_PREFIX=NIK_IPDr
export RECHARGELOG_TABLE_NAME_PREFIX=NIK_RECHARGE_LOG
export RATER_CDR_INPUTDIR=RaterCDRData/
export RATER_GPRS_INPUTDIR=RaterGPRSCDRData/
export RATER_CDR_OUTPUTDIR=RaterOutput/
export RATER_GPRS_OUTPUTDIR=RaterOutputGPRS/
export DB_TYPE=1
SPARK_REFERENCE_DB=(
 				ora11g
 				spark
 				spark
 				)

USAGEDB=(
		ora11g
		spark
		spark
		)


if [ $IS_DB2_ENABLED == 0 ]
     then
        export DB_QUERY="sqlplus -s ${DB_SETUP_USER}/${DB_SETUP_PASSWORD}"
        export GREP_QUERY=''
     else
        export DB_QUERY="clpplus -nw -s ${DB2_SERVER_USER}/${DB2_SERVER_PASSWORD}@${DB2_SERVER_HOST}:${DB2_INSTANCE_PORT}/${DB2_DATABASE_NAME}"
        export GREP_QUERY='grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."'
fi

