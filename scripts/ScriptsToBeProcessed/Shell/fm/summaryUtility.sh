#!/usr/bin/env bash

source $RANGERHOME/bin/utility.sh

runSummaryScript()
{
	summaryDayOfYear="$1"
	summaryHourOfDay="$2"
	recordConfigID=$3

	if [ $recordConfigID -eq 1 ]
	then
		insertRunSummaryEntry
	else
		return
	fi
}

# Insert into RUN_SUMMARY table
insertRunSummaryEntry()
{
	$SQL_COMMAND -s /nolog << EOF > $tempsqloutput  2>&1
	CONNECT_TO_SQL
	WHENEVER OSERROR  EXIT 6 ;
	WHENEVER SQLERROR EXIT 5 ;
		INSERT INTO RUN_SUMMARY VALUES (RUN_SUMMARY_ID.NEXTVAL, $summaryDayOfYear, '$summaryHourOfDay') ;
		COMMIT ;
EOF
	sqlretcode=`echo $?`
	cat $tempsqloutput | grep "1 row created." > /dev/null 2>&1
	if [ $sqlretcode -eq 0 -a "$?" -ne 0 ]
	then
		errorMessage="Statement failed:\nINSERT INTO RUN_SUMMARY VALUES (RUN_SUMMARY_ID.NEXTVAL, $summaryDayOfYear,'$summaryHourOfDay')"
		logErrorMessage "$errorMessage"
	fi
	sqlErrorCheck $sqlretcode
}

