#!/bin/bash

function CheckParam()
{	
    echo "Checking parameters of DB"
	rm -f -- *.log
	
	if [ -z $DB_USER ]
	then
		echo 'DB_USER Not Set. Please set the environment variable DB_USER'
		exit 1
	fi
	
	if [ -z $DB_PASSWORD ]
	then
		echo 'DB_PASSWORD Not Set. Please set the environment variable DB_PASSWORD'
		exit 1
	fi
	 
	if [ -z $NIKIRA_ORACLE_SID ]
	then
		echo 'NIKIRA_ORACLE_SID Not Set. Please set the environment variable NIKIRA_ORACLE_SID'
		exit 1
	fi
 
	Nikira_DB_User=$DB_USER
	Nikira_DB_Password=$DB_PASSWORD
	Nikira_Oracle_SID=$NIKIRA_ORACLE_SID
	echo "the Nikira_DB_User= $Nikira_DB_User and Nikira_DB_Password=$Nikira_DB_Password"

	rm -f *.log
    

	
}


function Migration_SP2_to_SP3()
{
	sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >>Migration_SP3_PATCH_4_SP3_DDL.log
   		set heading on ;
   	 	set feedback on ;
   	 	set serveroutput on ;
   	 	whenever oserror exit 5 ;
   	 	
	    @schema.sql;
		COMMIT;
/
EOF
	
	
	
	sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >>Migration_SP3_PATCH_4_SP3_DML.log
   		set heading on ;
   	 	set feedback on ;
   	 	set serveroutput on ;
   	 	whenever sqlerror exit 5 rollback ;
   	 	whenever oserror exit 5 ;
   	 	
		@RC_RECORD_CONFIGS.sql;
		@RECORD_VIEW_CONFIGS.sql;
		@SCHEDULER_COMMAND_MAPS.sql;
		@SOURCE.sql;
		@AUDIT_LOG_EVENT_CODES.sql;
		@BASIC_FILTER_CONFIGS.sql;
		@EXPANDABLE_FIELD_MAPS_RJIL.sql;
		@FEATURE_CONF_TAKS.sql;
		@TASKS.sql;
		@FEATURE_TASK_MAPS.sql;
		@RANGER_GROUPS_TASKS.sql;
		@HELP_FILES_MAPS.sql;
		@RECORD_CONFIGS.sql;
		@FIELD_PERF_SPEED_MAPPINGS.sql;
		@DYNAMIC_QUERY_PATHS.sql;
		commit;
   	 	
/   	 	
EOF

}


function Main()
{
	CheckParam
	Migration_SP2_to_SP3
}
Main
