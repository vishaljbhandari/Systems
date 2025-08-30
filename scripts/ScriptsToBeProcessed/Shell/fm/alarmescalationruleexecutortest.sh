#! /bin/sh

. shUnit


function TestAlarmEscalationRuleExecutionSuccessWithParameter
{
	shuAssert "Setup" "scriptlauncher ./Tests/alarmescalationruleexecutorsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./AlarmEscalationRuleExecutor.sh ALARM_ESCALATION_RULE" 
	shuAssert "Verification" "scriptlauncher ./Tests/alarmescalationruleexecutorsuccessV.sh" 
} > /dev/null 2>&1 

function TestAlarmEscalationRuleExecutionSuccessWithoutParameter
{
	shuAssert "Setup" "scriptlauncher ./Tests/alarmescalationruleexecutorsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./AlarmEscalationRuleExecutor.sh"
	shuAssert "Verification" "scriptlauncher ./Tests/alarmescalationruleexecutorsuccessV.sh" 
} > /dev/null 2>&1 

function TestAlarmEscalationRuleExecutionFailureForExecutableNotPresent
{
	if [ -f $RANGERHOME/sbin/recorddispatcher ]
    then
		`mv $RANGERHOME/sbin/recorddispatcher $RANGERHOME/sbin/recorddispatcher1`
		FLAG=1
    fi

	shuAssert "Setup" "scriptlauncher ./Tests/alarmescalationruleexecutorsetup.sh" 
	`scriptlauncher ./AlarmEscalationRuleExecutor.sh`
	shuAssert "Verification" "scriptlauncher ./Tests/alarmescalationruleexecutorfailureV.sh" 

	if [ $FLAG -eq 1 ]
    then
		`mv $RANGERHOME/sbin/recorddispatcher1 $RANGERHOME/sbin/recorddispatcher`
    fi
} > /dev/null 2>&1 

function InitFunction
{
	export TEST_RUN="1"

	shuRegTest TestAlarmEscalationRuleExecutionSuccessWithParameter
	shuRegTest TestAlarmEscalationRuleExecutionSuccessWithoutParameter
	shuRegTest TestAlarmEscalationRuleExecutionFailureForExecutableNotPresent
}

shuStart InitFunction
