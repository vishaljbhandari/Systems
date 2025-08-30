#! /bin/sh

NON_FRAUD_SUBSCRIBER=0
FRAUD_SUBSCRIBER=1
PROFILE_PROCESS=0
BLACKLIST_PROFILE_PROCESS=1


. shUnit

function TestMoveToHotlistAndBlackList
{
	shuAssert "Setup" "scriptlauncher ./Tests/alarmclosureactionmovetohotlistsetup.sh" 
	shuAssert "Running script" "ruby RubyScripts/alarm_closure.rb 1025" 
	shuAssert "Verification" "scriptlauncher ./Tests/alarmclosureactionmovetohotlistV.sh" 
} > /dev/null 2>&1 

function TestCalledToAlarmGeneration
{
	shuAssert "Setup" "scriptlauncher ./Tests/calledtoalarmgenerationsetup.sh" 
	shuAssert "Running script" "ruby RubyScripts/alarm_closure.rb 1024" 
	shuAssert "Verification" "scriptlauncher ./Tests/calledtoalarmgenerationV.sh" 
} > /dev/null 2>&1 

function TestPopulateAlertCDRs
{
	shuAssert "Setup" "scriptlauncher ./Tests/testpopulatealertcdrs.sh" 
	shuAssert "Running script" "ruby RubyScripts/alarm_closure.rb 1024" 
	shuAssert "Verification" "scriptlauncher ./Tests/testpopulatealertcdrsV.sh" 
} > /dev/null 2>&1 

function TestCalledByAlarmGeneration
{
	shuAssert "Setup" "scriptlauncher ./Tests/calledbyalarmgenerationsetup.sh" 
	shuAssert "Running script" "ruby RubyScripts/alarm_closure.rb 1024" 
	shuAssert "Verification" "scriptlauncher ./Tests/calledbyalarmgenerationV.sh" 
} > /dev/null 2>&1 

function TestASNWorldNumbers
{
    shuAssert "Setup" "scriptlauncher ./Tests/asnworldnumbersetup.sh "
    shuAssert "Running script" "ruby RubyScripts/alarm_closure.rb 1024"
    shuAssert "Verification" "scriptlauncher ./Tests/asnworldnumber_verification.sh"
}> /dev/null 2>&1

function InitFunction
{
	export TEST_RUN="1"

	shuRegTest TestMoveToHotlistAndBlackList
	shuRegTest TestCalledToAlarmGeneration
	shuRegTest TestPopulateAlertCDRs
	shuRegTest TestCalledByAlarmGeneration
	shuRegTest TestASNWorldNumbers
}

shuStart InitFunction
