#! /bin/sh

. shUnit


function TestAlarmReminderRuleExecutionSuccessWithParameter
{
	shuAssert "Setup" "scriptlauncher ./Tests/alarmreminderruleexecutorsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./AlarmReminderRuleExecutor.sh ALARM_REMINDER_RULE" 
	shuAssert "Verification" "scriptlauncher ./Tests/alarmreminderruleexecutorsuccessV.sh" 
} > /dev/null 2>&1 

function TestAlarmReminderRuleExecutionSuccessWithoutParameter
{
	shuAssert "Setup" "scriptlauncher ./Tests/alarmreminderruleexecutorsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./AlarmReminderRuleExecutor.sh"
	shuAssert "Verification" "scriptlauncher ./Tests/alarmreminderruleexecutorsuccessV.sh" 
} > /dev/null 2>&1 

function TestAlarmReminderRuleExecutionFailureForExecutableNotPresent
{
	if [ -f $RANGERHOME/sbin/recorddispatcher ]
    then
		`mv $RANGERHOME/sbin/recorddispatcher $RANGERHOME/sbin/recorddispatcher1`
		FLAG=1
    fi

	shuAssert "Setup" "scriptlauncher ./Tests/alarmreminderruleexecutorsetup.sh" 
	`scriptlauncher ./AlarmReminderRuleExecutor.sh`
	shuAssert "Verification" "scriptlauncher ./Tests/alarmreminderruleexecutorfailureV.sh" 

	if [ $FLAG -eq 1 ]
    then
		`mv $RANGERHOME/sbin/recorddispatcher1 $RANGERHOME/sbin/recorddispatcher`
    fi
} > /dev/null 2>&1 

function InitFunction
{
	export TEST_RUN="1"

	shuRegTest TestAlarmReminderRuleExecutionSuccessWithParameter
	shuRegTest TestAlarmReminderRuleExecutionSuccessWithoutParameter
	shuRegTest TestAlarmReminderRuleExecutionFailureForExecutableNotPresent
}

shuStart InitFunction
