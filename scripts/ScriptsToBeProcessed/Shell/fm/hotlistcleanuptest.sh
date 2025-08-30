#! /bin/sh
. shUnit

function TestHotlistCleanup
{
    shuAssert "Setup script" "scriptlauncher ./Tests/testHotlistCleanupSetup.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./hotlistcleanup.sh" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testHotlistCleanupV.sh"  > /dev/null 2>&1
}	

function InitFunction
{
	shuRegTest TestHotlistCleanup 
}
shuStart InitFunction
