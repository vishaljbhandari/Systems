#! /bin/sh

. shUnit

function TestArchivedAlarmCleanup
{
	shuAssert "Setup" "scriptlauncher ./Tests/testarchivedalarmcleanupsetup.sh"
	shuAssert "Running script" "scriptlauncher ./archivedalarmcleanup.sh"
	shuAssert "Verification" "scriptlauncher ./Tests/testarchivedalarmcleanupV1.sh"
} > /dev/null 2>&1

function InitFunction
{
	shuRegTest TestArchivedAlarmCleanup
}

shuStart InitFunction
