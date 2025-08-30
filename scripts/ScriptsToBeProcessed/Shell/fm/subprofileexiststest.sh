
#! /bin/sh
. shUnit

function TestAIGroups
{
    shuAssert "Setup" "scriptlauncher ./Tests/testAIGroups.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./subprofileexists.sh 0" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testAIGroupsV.sh"  > /dev/null 2>&1
}

function TestOneTimeAddition
{
    shuAssert "Setup" "scriptlauncher ./Tests/testOneTimeAddition.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./subprofileexists.sh 0" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testOneTimeAdditionV.sh"  > /dev/null 2>&1
}

function TestEnableProfiling
{
    shuAssert "Setup" "scriptlauncher ./Tests/testEnableCumulativeVoiceProfiling.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./subprofileexists.sh 0" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testEnableCumulativeVoiceProfilingV.sh"  > /dev/null 2>&1

    shuAssert "Setup" "scriptlauncher ./Tests/testEnableCumulativeDataProfiling.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./subprofileexists.sh 0" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testEnableCumulativeDataProfilingV.sh"  > /dev/null 2>&1

    shuAssert "Setup" "scriptlauncher ./Tests/testEnableCumulativeROCVoiceProfiling.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./subprofileexists.sh 0" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testEnableCumulativeROCVoiceProfilingV.sh"  > /dev/null 2>&1

    shuAssert "Setup" "scriptlauncher ./Tests/testEnableCumulativeROCDataProfiling.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./subprofileexists.sh 0" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testEnableCumulativeROCDataProfilingV.sh"  > /dev/null 2>&1
}

function TestNetworkSpecificPopulation
{
	shuAssert "Setup" "scriptlauncher ./Tests/testNetworkSpecificPopulation.sh" > /dev/null 2>&1
	shuAssert "Running script" "scriptlauncher ./subprofileexists.sh 1" > /dev/null 2>&1
	shuAssert "Verification" "scriptlauncher ./Tests/testNetworkSpecificPopulationV.sh" > /dev/null 2>&1
}

function InitFunction
{
	shuRegTest TestAIGroups
	shuRegTest TestOneTimeAddition
	shuRegTest TestEnableProfiling
	shuRegTest TestNetworkSpecificPopulation
}

shuStart InitFunction

