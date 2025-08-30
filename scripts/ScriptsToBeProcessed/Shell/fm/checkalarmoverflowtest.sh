#! /bin/sh
. shUnit

function TestTotalAlertCount
{
    shuAssert "Setup" "scriptlauncher ./Tests/testtotalalertcountsetup.sh"
    shuAssert "Running script" "scriptlauncher ./checkalarmoverflow.sh 0 2"
	shuAssert "Verification" "scriptlauncher ./Tests/testtotalalertcountV.sh"
} > /dev/null 2>&1

function TestTreeBaseEvent
{
    shuAssert "Setup" "scriptlauncher ./Tests/testtreebaseeventsetup.sh"
    shuAssert "Running script" "scriptlauncher ./checkalarmoverflow.sh 0 2"
	shuAssert "Verification" "scriptlauncher ./Tests/testtreebaseeventV.sh"
} > /dev/null 2>&1

function TestNormalBaseEvent
{
    shuAssert "Setup" "scriptlauncher ./Tests/testnormalbaseeventsetup.sh"
    shuAssert "Running script" "scriptlauncher ./checkalarmoverflow.sh 0 2"
	shuAssert "Verification" "scriptlauncher ./Tests/testnormalbaseeventV.sh"
} > /dev/null 2>&1

function InitFunction
{
	shuRegTest TestTotalAlertCount
	shuRegTest TestTreeBaseEvent 
	shuRegTest TestNormalBaseEvent
}

shuStart InitFunction
