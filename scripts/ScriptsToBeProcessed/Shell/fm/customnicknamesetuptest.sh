#! /bin/sh
. shUnit

function TestCustomNicknameSetup
{
    shuAssert "Setup script" "scriptlauncher ./Tests/testCustomNicknameSetup.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./customnicknamesetup.sh Test DAILYADDITION 2 ALL" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testCustomNicknameSetupV.sh"  > /dev/null 2>&1
}	

function TestCustomNicknameSetupStandalone
{
    shuAssert "Setup script" "scriptlauncher ./Tests/testCustomNicknameSetup.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./customnicknamesetup.sh Test DAILYADDITION 2 STANDALONE" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testCustomNicknameSetupVStandalone.sh" > /dev/null 2>&1
}	

function TestCustomNickNameForSimilarEntries
{
	shuAssert "Setup script" "scriptlauncher ./Tests/testCustomNicknameSetup.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./customnicknamesetup.sh NickNick DAILYADDITION 2 ALL" > /dev/null 2>&1
	shuAssert "Verification 1" "scriptlauncher ./Tests/testCustomNickNameForSimilarEntriesV.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./customnicknamesetup.sh Nick DAILYADDITION 2 ALL" > /dev/null 2>&1
    shuAssert "Verification 2" "scriptlauncher ./Tests/testCustomNickNameForSimilarEntriesV1.sh" > /dev/null 2>&1

}

function InitFunction
{
	shuRegTest TestCustomNicknameSetup 
	shuRegTest TestCustomNicknameSetupStandalone 
	shuRegTest TestCustomNickNameForSimilarEntries
}
shuStart InitFunction

