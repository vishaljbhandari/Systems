#! /bin/sh

. shUnit

function TestCalledToAlarmGeneration
{
	shuAssert "Setup" "scriptlauncher ./Tests/calledtoalarmgenerationsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./AlarmClosureActionScheduled.sh 1024" 
	shuAssert "Verification" "scriptlauncher ./Tests/calledtoalarmgenerationV.sh" 
} > /dev/null 2>&1 

function TestPopulateAlertCDRs
{
	shuAssert "Setup" "scriptlauncher ./Tests/testpopulatealertcdrs.sh" 
	shuAssert "Running script" "scriptlauncher ./AlarmClosureActionScheduled.sh 1024" 
	shuAssert "Verification" "scriptlauncher ./Tests/testpopulatealertcdrsV.sh" 
} > /dev/null 2>&1 

function TestCalledByAlarmGeneration
{
	shuAssert "Setup" "scriptlauncher ./Tests/calledbyalarmgenerationsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./AlarmClosureActionScheduled.sh 1024" 
	shuAssert "Verification" "scriptlauncher ./Tests/calledbyalarmgenerationV.sh" 
} > /dev/null 2>&1 

function InitFunction
{
	export TEST_RUN="1"
	shuRegTest TestCalledToAlarmGeneration
	shuRegTest TestPopulateAlertCDRs
	shuRegTest TestCalledByAlarmGeneration
}

shuStart InitFunction
