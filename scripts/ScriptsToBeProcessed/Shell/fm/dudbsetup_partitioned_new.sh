#! /bin/bash

#. rangerenv.sh
DBScriptsFolder=$DBSETUP_HOME
PartitionedDBScriptsFolder=$DBSETUP_HOME/DBSetup_Partitioned
export DBScriptsFolder
FILES="	$DBScriptsFolder/ranger-new-helper-functions.sql
		$DBScriptsFolder/joinmanyrowsin1.sql
		ranger-db-DU.sql
		ranger-db-DU-ss7.sql
		ranger-db-DU-SummaryTables.sql
		roamer-partner-setup.sql
		$PartitionedDBScriptsFolder/nikira-db-with-tablespace-COUNTERS.sql
		nikira-db-with-tablespace-NETFLOW-COUNTERS.sql
		$PartitionedDBScriptsFolder/ranger-new-db-with-tablespaces.sql
 		$PartitionedDBScriptsFolder/ranger-gprs-db.sql
		$PartitionedDBScriptsFolder/ranger-sp-db.sql
		$PartitionedDBScriptsFolder/workflow-db.sql
		$DBScriptsFolder/workflow-db-exec.sql
		$PartitionedDBScriptsFolder/summary_tables.sql
		$PartitionedDBScriptsFolder/ranger-db-Rater.sql
		$DBScriptsFolder/ranger-new-triggers.sql
		$DBScriptsFolder/ranger-new-internal_user.sql

		$DBScriptsFolder/ranger-new-field-categories.sql
		$DBScriptsFolder/ranger-new-record-config.sql
		../ranger-new-record-config-DU.sql
		$DBScriptsFolder/ranger-new-exec.sql
		../ranger-new-exec-DU.sql
		../ranger-exec-DU.sql
		../ranger-gprs-exec-DU.sql
		../ranger-exec-du-reportnamemap.sql
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
		$PartitionedDBScriptsFolder/ranger-new-db-AI.sql
		$DBScriptsFolder/ranger-new-exec-AI.sql
		$PartitionedDBScriptsFolder/MachineLearning/mlhmm-db.sql
        $PartitionedDBScriptsFolder/MachineLearning/mlaqhm-db.sql
        $PartitionedDBScriptsFolder/MachineLearning/mlaqhmminterface-db.sql
        $DBScriptsFolder/MachineLearning/mlaqhmmband-exec.sql
        $DBScriptsFolder/MachineLearning/mlaqhmm-exec.sql

		$DBScriptsFolder/ranger-exec-Rater.sql
		$DBScriptsFolder/ranger-vpmn-rules.sql
		$DBScriptsFolder/ranger-packages-procedures.sql
		$DBScriptsFolder/ranger-negative-rule-exec.sql
		$DBScriptsFolder/internal-user-exec.sql
		$DBScriptsFolder/ranger-sp-st-rule.sql

		../ranger-net-flow-exec-DU.sql
		../rater-db-views.sql
		../ranger-du-ds-field-config.sql
		../DS/duglobaldata.sql
		../DS/dusmscdrds.sql
		../DS/ducdrds.sql
		../DS/dusubscriberds.sql
		../DS/duEDCHds.sql
		../DS/dummscdrds.sql
		../DS/dugprsds.sql
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

echo
echo "DB Setup Completed."
