#! /bin/sh

. shUnit

function TestSubscriberCleanup
{
    shuAssert "Setup" "scriptlauncher ./Tests/testsubscribercleanupsetup.sh" 
    shuAssert "Running script" "scriptlauncher ./disconnectedsubscribercleanup.sh 1024" 
    shuAssert "Verification" "scriptlauncher ./Tests/testsubscribercleanupV.sh" 
} > /dev/null 2>&1

function TestFingerprintProfileCleanup
{
    shuAssert "Setup" "scriptlauncher ./Tests/testsubscribercleanupsetup.sh" 
    shuAssert "Running script" "scriptlauncher ./disconnectedsubscribercleanup.sh 1024" 
    shuAssert "Verification" "scriptlauncher ./Tests/testfingerprintcleanupV.sh" 
} > /dev/null 2>&1

function InitFunction
{
    shuRegTest TestSubscriberCleanup
	shuRegTest TestFingerprintProfileCleanup
}

shuStart InitFunction
