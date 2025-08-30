#! /bin/sh

. shUnit

function TestCleanupAlarms
{
	shuAssert "Setup" "scriptlauncher ./Tests/testcleanupalarmssetup.sh" 
	shuAssert "Running script" "scriptlauncher ./cleanupAlarms.sh" 
	shuAssert "Verification" "scriptlauncher ./Tests/testcleanupalarmsV.sh"
} > /dev/null 2>&1

function InitFunction
{
	shuRegTest TestCleanupAlarms
}

shuStart InitFunction
