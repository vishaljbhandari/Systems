#! /bin/sh

. shUnit

function TestFingerprintRuleExecutionFailureWrongParameters
{
	shuAssert "Setup" "scriptlauncher ./Tests/fingerprintruleexecutorsetup.sh" 
	shuAssertFalse "Running script" "scriptlauncher ./FingerprintRuleExecutor.sh" 
	shuAssert "Verification" "scriptlauncher ./Tests/fingerprintruleexecutorfailureV.sh" 
} > /dev/null 2>&1 

function TestFingerprintRuleExecutionSuccessForBulk
{
	ENTITY_TYPE=16
	NETWORK_ID=1024
	shuAssert "Setup" "scriptlauncher ./Tests/fingerprintruleexecutorsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./FingerprintRuleExecutor.sh $NETWORK_ID $ENTITY_TYPE" 
	shuAssert "Verification" "scriptlauncher ./Tests/fingerprintruleexecutorsuccessV.sh" 
} > /dev/null 2>&1 

function TestFingerprintRuleExecutionSuccessForSingleEntity
{
	ENTITY_TYPE=16
	SUBSCRIBER_ID=1024
	RULE_CATEGORY="MANUAL_PROFILE_MATCH"
	shuAssert "Setup" "scriptlauncher ./Tests/fingerprintruleexecutorsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./FingerprintRuleExecutor.sh $ENTITY_TYPE $SUBSCRIBER_ID $RULE_CATEGORY" 
	shuAssert "Verification" "scriptlauncher ./Tests/fingerprintruleexecutorsuccessV.sh" 
} 
> /dev/null 2>&1 

function InitFunction
{
	export TEST_RUN="1"

	shuRegTest TestFingerprintRuleExecutionFailureWrongParameters
	shuRegTest TestFingerprintRuleExecutionSuccessForBulk
	shuRegTest TestFingerprintRuleExecutionSuccessForSingleEntity
}

shuStart InitFunction
