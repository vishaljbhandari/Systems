#! /bin/sh

. shUnit

function TestNegativeRuleExecutionSuccess
{
	RULE_ID=7000
	NETWORK_ID=1024
	shuAssert "Setup" "scriptlauncher ./Tests/negativeruleexecutionsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./NegativeRuleExecutor.sh $NETWORK_ID $RULE_ID" 
	shuAssert "Verification" "scriptlauncher ./Tests/negativeruleexecutionV.sh" 
} > /dev/null 2>&1 

function TestNegativeRuleExecutionFailureWrongParameters
{
	shuAssertFalse "Running script" "scriptlauncher ./NegativeRuleExecutor.sh " 
} > /dev/null 2>&1 

function InitFunction
{
	export TEST_RUN="1"
	shuRegTest TestNegativeRuleExecutionSuccess
	shuRegTest TestNegativeRuleExecutionFailureWrongParameters
}

shuStart InitFunction
