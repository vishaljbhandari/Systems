#! /bin/sh
. shUnit

NETWORK_ID=1025

function TestCustomNicknameLoader
{
	`echo "+9198943232" > Test1025.txt`
  	`rm -rf $COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/Test_2`
 	`mkdir -p $COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/Test_2`
 	`mkdir -p $COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/Test_2/new`
  	`cp -f Test1025.txt $COMMON_MOUNT_POINT/FMSData/CustomNicknamesData/Test_2/new/`
      shuAssert "Setup script" "scriptlauncher ./Tests/testCustomNicknameLoaderSetup.sh" > /dev/null 2>&1
      shuAssert "Running Setup script" "scriptlauncher ./customnicknamesetup.sh Test DAILYADDITION 2" > /dev/null 2>&1
    shuAssert "Setup script" "scriptlauncher ./Tests/testCustomNicknameLoaderSetup.sh" > /dev/null 2>&1
    shuAssert "Running Setup script" "scriptlauncher ./customnicknamesetup.sh Test DAILYADDITION 2 ALL" > /dev/null 2>&1
    shuAssert "Running Load script" "scriptlauncher ./customnicknameload.sh $NETWORK_ID Test 2 DAILYADDITION" > /dev/null 2>&1 
    shuAssert "Verification" "scriptlauncher ./Tests/testCustomNicknameLoader.sh"  > /dev/null 2>&1
	`rm -rf Test1025.txt`
}	

function InitFunction
{
	shuRegTest TestCustomNicknameLoader 
}
shuStart InitFunction

