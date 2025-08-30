#! /bin/sh

. shUnit

function TestArchiveAlarms
{
	shuAssert "Setup" "scriptlauncher ./Tests/testarchivealarmssetup.sh" 
	shuAssert "Running script" "scriptlauncher ./archivealarms.sh"
	shuAssert "Verification" "scriptlauncher ./Tests/testarchivealarmsV.sh"
}  > /dev/null 2>&1

function TestArchiveAlarmsFpProfiles
{
	shuAssert "Setup" "scriptlauncher ./Tests/testarchivefingerprintalarmssetup.sh" > /dev/null 2>&1 
	shuAssert "Running script" "scriptlauncher ./archivealarms.sh" > /dev/null 2>&1
	shuAssert "Verification" "scriptlauncher ./Tests/testarchivefpalarmsV.sh" > /dev/null 2>&1
}

function InitFunction
{
	shuRegTest TestArchiveAlarms
	shuRegTest TestArchiveAlarmsFpProfiles
}

shuStart InitFunction
