#! /bin/bash

. rangerenv.sh

DBScriptsFolder=/home/umam/Workspace/v6GBXBranch/DBSetup
export DBScriptsFolder

FILES=" nikira-db-telus.sql
		$DBScriptsFolder/ranger-new-db.sql
		$DBScriptsFolder/ranger-sp-db.sql
		$DBScriptsFolder/summary_tables.sql
		$DBScriptsFolder/ranger-db-Rater.sql


		$DBScriptsFolder/ranger-new-triggers.sql
		$DBScriptsFolder/ranger-packages-procedures.sql

		$DBScriptsFolder/ranger-new-field-categories.sql
		$DBScriptsFolder/ranger-new-record-config.sql
		nikira-new-record-config-telus.sql
		nikira-aggregation-type-map-entries.sql
		$DBScriptsFolder/ranger-new-exec.sql
		nikira-new-exec-telus.sql
		$DBScriptsFolder/ranger-counter_map_entries.sql
		
		$DBScriptsFolder/ranger-rule-exec.sql
		$DBScriptsFolder/ranger-new-task-exec.sql
		$DBScriptsFolder/ranger-subscriber-store-rules.sql
		$DBScriptsFolder/ranger-match-function-exec.sql

		$DBScriptsFolder/ranger-exec-Rater.sql

		nikira-telus-specific-tables.sql
		nikira-telus-specific-data.sql "
User=$1
Password=$2

. oraenv ;

if [ "$Password" = "" ]; then
	Password=$User ;
fi

if [ "$User" = "" ]; then
	User=$DB_USER ;
	Password=$DB_PASSWORD ;
fi

for file in $FILES
do
	echo "Running $file"
	sqlplus -l -s $User/$Password < $file
	if [ $? -ne 0 ]; then
		exit 1 ;
	fi
done

echo
echo "DB Setup Completed."
