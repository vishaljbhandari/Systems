#! /bin/sh

. shUnit

function TestCleanupRecords
{
	shuAssert "Setup" "scriptlauncher ./Tests/testcleanupRecordssetup.sh"
	shuAssert "Running script" "scriptlauncher ./cleanupRecords.sh"
	shuAssert "Verification" "scriptlauncher ./Tests/testcleanupRecordsV.sh"
} > /dev/null 2>&1

function InitFunction
{
	export TEST_RUN=1
	shuRegTest TestCleanupRecords
}

shuStart InitFunction
