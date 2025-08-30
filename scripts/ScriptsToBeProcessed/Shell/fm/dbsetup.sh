if [ "$1" == "" ]
then
    echo "Invalid parameters."
    echo "Usage : dbsetup.sh <nikira_db_user> <nikira_db_password>"
	exit ;
fi
if [ "$2" == "" ]
then
    echo "Invalid parameters."
    echo "Usage : dbsetup.sh <nikira_db_user> <nikira_db_password>"
	exit ;
fi

echo "Running post scripts."
FM_USER=$1
FM_DB_Passwd=$2
FM_ORACLE_SID=$3
SPARK_DB_USER=$4
SPARK_DB_PASSWORD=$5
SPARK_ORACLE_SID=$6
FMS_DB_LINK=$7
SPARK_DB_LINK=$8


./spark_trend_summary_DDL_setup.sh $FM_USER $FM_DB_Passwd $FM_ORACLE_SID $SPARK_DB_USER $SPARK_DB_PASSWORD $SPARK_ORACLE_SID $SPARK_DB_LINK $FMS_DB_LINK > spark_trend_summary_setup.log
./sparkdbsetup.sh $FM_USER $FM_DB_Passwd $FM_ORACLE_SID $SPARK_DB_USER $SPARK_DB_PASSWORD $SPARK_ORACLE_SID $SPARK_DB_LINK $FMS_DB_LINK> sparkdbsetup.log
./sparkBugFixes.sh $FM_USER $FM_DB_Passwd $FM_ORACLE_SID $SPARK_DB_USER $SPARK_DB_PASSWORD $SPARK_ORACLE_SID $SPARK_DB_LINK $FMS_DB_LINK
./SparkLookupCDM.sh $FM_USER $FM_DB_Passwd $FM_ORACLE_SID $SPARK_DB_USER $SPARK_DB_PASSWORD $SPARK_ORACLE_SID $SPARK_DB_LINK $FMS_DB_LINK > sparkLookupCDM.log

echo "DBSetup completed."
