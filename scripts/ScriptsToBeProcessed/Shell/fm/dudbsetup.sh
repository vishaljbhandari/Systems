#! /bin/bash

. rangerenv.sh
DBScriptsFolder=$DBSETUP_HOME
export DBScriptsFolder

FILES=" $DBScriptsFolder/ranger-new-helper-functions.sql
		$DBScriptsFolder/joinmanyrowsin1.sql
		ranger-db-DU.sql
 		ranger-gprs-db-DU.sql
		ranger-db-DU-ss7.sql
		ranger-db-DU-SummaryTables.sql
		roamer-partner-setup.sql
		$DBScriptsFolder/ranger-new-db.sql
 		$DBScriptsFolder/ranger-gprs-db.sql
		$DBScriptsFolder/ranger-sp-db.sql
		$DBScriptsFolder/workflow-db.sql
		$DBScriptsFolder/workflow-db-exec.sql
		$DBScriptsFolder/summary_tables.sql
		$DBScriptsFolder/ranger-db-Rater.sql
		$DBScriptsFolder/ranger-new-triggers.sql
		$DBScriptsFolder/ranger-new-internal_user.sql

		$DBScriptsFolder/ranger-new-field-categories.sql
		$DBScriptsFolder/ranger-new-record-config.sql
		ranger-new-record-config-DU.sql
		$DBScriptsFolder/ranger-new-exec.sql
        ranger-db-DU-Custamization.sql
		ranger-new-exec-DU.sql
 		ranger-gprs-exec-DU.sql
		ranger-exec-du-reportnamemap.sql
		$DBScriptsFolder/ranger-sp-exec.sql
		$DBScriptsFolder/ranger-counter_map_entries.sql

		$DBScriptsFolder/coloring_rules.sql
		$DBScriptsFolder/ranger-rule-exec.sql
		$DBScriptsFolder/ranger-reports-exec.sql
		$DBScriptsFolder/ranger-new-task-exec.sql
		$DBScriptsFolder/ranger-subscriber-store-rules.sql
		$DBScriptsFolder/ranger-MLH-store-rules.sql
		$DBScriptsFolder/ranger-match-function-exec.sql
		$DBScriptsFolder/ranger-precheck-entries.sql
		$DBScriptsFolder/ranger-new-db-AI.sql
		$DBScriptsFolder/ranger-new-exec-AI.sql
		$DBScriptsFolder/MachineLearning/ml-setup.sql
		$DBScriptsFolder/ranger-exec-Rater.sql
		$DBScriptsFolder/ranger-vpmn-rules.sql
		$DBScriptsFolder/ranger-packages-procedures.sql 
		$DBScriptsFolder/ranger-negative-rule-exec.sql
		$DBScriptsFolder/internal-user-exec.sql
		$DBScriptsFolder/ranger-sp-st-rule.sql

		rater-db-views.sql
		ranger-du-ds-field-config.sql
		DS/duglobaldata.sql
		DS/dusmscdrds.sql
		DS/ducdrds.sql
		DS/dusubscriberds.sql
		DS/duEDCHds.sql
		DS/dummscdrds.sql
		DS/dugprsds.sql
		ranger-exec-DU.sql
		ranger-DU-translations.sql
		ranger-DU-reports.sql
		ranger-DU-scheduler-entries.sql
       	ranger-db-DU-Custamization.sql
		ranger-net-flow-DU.sql
		ranger-net-flow-exec-DU.sql
		ranger-pgw-exec-DU.sql
		ranger-DU-ss7.sql
		ranger-DU-fieldutil.sql
		"


User=$1
Password=$2

RunPath=`dirname $0`

. oraenv ;

if [ "$Password" = "" ]; then
	Password=$User ;
fi

if [ "$User" = "" ]; then
	User=$DB_USER ;
	Password=$DB_PASSWORD ;
fi

cd $RunPath

echo "Running Revmax-DBSetup.sh"
sh revmax/revmaxsetup.sh
if [ $? -ne 0 ]; then
    exit 18
fi

for file in $FILES
do
	echo "Running $file"
	sqlplus -l -s $User/$Password < $file
	if [ $? -ne 0 ]; then
		exit 1 ;
	fi
done

. $RANGERHOME/sbin/rangerenv.sh
ruby $DBScriptsFolder/task-delegater.rb 0 $User $Password $DBScriptsFolder

echo
echo "DB Setup Completed."
