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
    
	common_mount_point=$COMMON_MOUNT_POINT

	grep -irl '@commonmountpoint@' IN_MEMORY_COUNTER_DETAILS.sql | xargs sed -i 's#@commonmountpoint@#'"$common_mount_point"'#g'
	
}


function Migration_SP2_to_SP3()
{
	sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >>Migration_SP2_SP3_DDL.log
   		set heading on ;
   	 	set feedback on ;
   	 	set serveroutput on ;
   	 	whenever oserror exit 5 ;
   	 	
		@DropAlmSeqTbl.sql;
		@schema.sql;
		@viewcreation.sql;
		COMMIT;
/
EOF
	
	
	
	sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >>Migration_SP2_SP3_DML.log
   		set heading on ;
   	 	set feedback on ;
   	 	set serveroutput on ;
   	 	whenever sqlerror exit 5 rollback ;
   	 	whenever oserror exit 5 ;
   	 	
		@RC_RECORD_CONFIGS.sql;
		@RULES.sql;
		@RECORD_CONFIGS_RULES.sql;
		@RECORD_VIEW_CONFIGS.sql;
		@SCHEDULER_COMMAND_MAPS.sql;
		@SOURCE.sql;
		@USER_OPTIONS.sql;
		@COUNTER_DETAILS.sql;
		@AUDIT_LOG_EVENT_CODES.sql;
		@BASIC_FILTER_CONFIGS.sql;
		@COUNTER_MANAGER_MAPPINGS.sql;
		@EXPANDABLE_FIELD_MAPS.sql;
		@FP_ENTITIES.sql;
		@FEATURE_CONFIGURATIONS.sql;
		@FEATURE_CONF_TAKS.sql;
		@TASKS.sql;
		@FEATURE_TASK_MAPS.sql;
		@FIELD_RECORD_CONFIG_MAPS.sql;
		@FP_CONFIGURATIONS.sql;
		@FP_FILTERED_ENTITIES_MAP.sql;
		@FP_MT_0_ENTITYID_MAP.sql;
		@FP_PROFILES_DATASET_MAP.sql;
		@FP_PROFILES_ENTITYID_MAP.sql;
		@IN_MEMORY_COUNTER_DETAILS.sql;
		@IMC_PAGE_POOL_DETAILS.sql;
		@IM_COUNTER_THREAD_CONFIGS.sql;
		@IN_MEMORY_BUFFER_CONFIGS.sql;
		@LINK_CHART_FILTERS.sql;
		@PROFILE_MANAGER_ENTITY_CONFIGS.sql;
		@PM_INSTANCE_CONFIGS.sql;
		@PROFILE_USER_CONFIGS.sql;
		@RANGER_GROUPS_TASKS.sql;
		@RULE_ACTION_PARAMETERS.sql;
		@FIELD_CATEGORIES.sql;
		@FIELD_CONFIGS.sql;
		@HELP_FILES_MAPS.sql;
		@RECORD_CONFIGS.sql;
		@CONFIGURATIONS.sql;
        @FIELD_PERF_SPEED_MAPPINGS.sql;
		@DYNAMIC_QUERY_PATHS.sql;
		@recompile_invalid.sql;
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
