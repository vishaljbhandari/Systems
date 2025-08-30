#! /bin/sh

. shUnit

function TestStatisticalRuleExecutionFailureWrongParameters
{
	shuAssert "Setup" "scriptlauncher ./Tests/statisticalruleexecutorsetup.sh" 
	shuAssertFalse "Running script" "scriptlauncher ./StatisticalRuleExecutor.sh" 
	shuAssert "Verification" "scriptlauncher ./Tests/statisticalruleexecutorfailureV.sh" 
} > /dev/null 2>&1 

function TestStatisticalRuleExecutionSuccessForBulk
{
	ENTITY_TYPE=2
	NETWORK_ID=1024
	shuAssert "Setup" "scriptlauncher ./Tests/statisticalruleexecutorsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./StatisticalRuleExecutor.sh $NETWORK_ID $ENTITY_TYPE" 
	shuAssert "Verification" "scriptlauncher ./Tests/statisticalruleexecutorsuccessV.sh" 
} > /dev/null 2>&1 

function TestStatisticalRuleExecutionSuccessForAllNetworks
{
	ENTITY_TYPE=2
	NETWORK_ID=999
	shuAssert "Setup" "scriptlauncher ./Tests/statisticalruleexecutorsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./StatisticalRuleExecutor.sh $NETWORK_ID $ENTITY_TYPE" 
	shuAssert "Verification" "scriptlauncher ./Tests/statisticalruleexecutorsuccessV.sh" 
} > /dev/null 2>&1 

function TestStatisticalRuleExecutionSuccessForSingleEntity
{
	ENTITY_TYPE=7
	ACCOUNT_ID=1024
	RULE_CATEGORY="STATISTICAL_RULE"
	shuAssert "Setup" "scriptlauncher ./Tests/statisticalruleexecutorsetup.sh" 
	shuAssert "Running script" "scriptlauncher ./StatisticalRuleExecutor.sh $ACCOUNT_ID $ENTITY_TYPE $RULE_CATEGORY" 
	shuAssert "Verification" "scriptlauncher ./Tests/statisticalruleexecutorsuccessV.sh" 
} > /dev/null 2>&1 

function InitFunction
{
	export TEST_RUN="1"

	shuRegTest TestStatisticalRuleExecutionFailureWrongParameters
	shuRegTest TestStatisticalRuleExecutionSuccessForSingleEntity
	shuRegTest TestStatisticalRuleExecutionSuccessForBulk
	shuRegTest TestStatisticalRuleExecutionSuccessForAllNetworks
}

shuStart InitFunction
