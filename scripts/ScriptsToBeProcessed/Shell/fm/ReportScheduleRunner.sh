#! /bin/bash
. rangerenv32.sh

NETWORK_ID=$1
SCHEDULER_ID=$2
RAILS_ROOT=$3
RAILS_ENV=$4
REPORT_ID=$5
RANGER_USER_ID=$6
    
function check_parameters
{
	NO_OF_PARAMETERS=$1

	if [ $NO_OF_PARAMETERS -ne 6 ] ; then
		echo "Usage: scriptlauncher ReportScheduleRunner.sh NetworkID SchedulerID RailsRoot RailsEnv ReportID RangerUserID"
		exit 2
	fi
}

NO_OF_PARAMETERS=$#
check_parameters $NO_OF_PARAMETERS


IS_SPARK_SUPPORT=`sqlplus -s $DB_USER/$DB_PASSWORD <<EOF  
    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF   
    select is_spark_support from record_configs where id =2 ; 
    EXIT 
EOF`

TABLE_NAME=''

if [ $IS_SPARK_SUPPORT != "1" ]
then 
TABLE_NAME=`sqlplus -s $DB_USER/$DB_PASSWORD <<EOF 
	    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF 
	    select tname from record_configs where id =2 ;
	    EXIT
EOF`
else 
TABLE_NAME_SUFFIX=`sqlplus -s $DB_USER/$DB_PASSWORD <<EOF                                                                                   
    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF 
    select report_table_name_seq.nextval from dual;
    EXIT
EOF`
TABLE_NAME='pin_reuse_'`echo $TABLE_NAME_SUFFIX | tr -d ' '`

fi

if [ $IS_SPARK_SUPPORT == "1" ]                                                                                                                              
then
$RAILS_ROOT/script/runner --environment=$RAILS_ENV $RAILS_ROOT/script/create_report_temp_table.rb $REPORT_ID $TABLE_NAME
sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD <<EOF 
    SET ECHO OFF NEWP 0 SPA 0 PAGES 0 FEED OFF HEAD OFF TRIMS ON TAB OFF 
	create or replace synonym $TABLE_NAME for $TABLE_NAME@$NIKIRA_DB_LINK;
	commit;
    EXIT 
EOF
fi

$RAILS_ROOT/script/runner --environment=$RAILS_ENV $RAILS_ROOT/script/scheduled_report_runner.rb $REPORT_ID $RANGER_USER_ID $NETWORK_ID $SCHEDULER_ID $TABLE_NAME $IS_SPARK_SUPPORT


if [ $IS_SPARK_SUPPORT == "1" ]                                                                                                                              
then
$RAILS_ROOT/script/runner --environment=$RAILS_ENV $RAILS_ROOT/script/drop_report_temp_table.rb $REPORT_ID $TABLE_NAME

sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD <<EOF 
	drop synonym $TABLE_NAME;
	commit;
    EXIT  
EOF
fi
