#! /bin/sh
. shUnit

function TestListConfigsPopulationForHotlist
{
    shuAssert "Setup script" "scriptlauncher ./Tests/testListConfigsPopulationForHotlistSetup.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./hotlistsetup.sh" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testListConfigsPopulationForHotlistV.sh"  > /dev/null 2>&1
}	

function TestListDetailsPopulationForHotlist
{
    shuAssert "Setup script" "scriptlauncher ./Tests/testListConfigsPopulationForHotlistSetup.sh" > /dev/null 2>&1
    shuAssert "Running script" "scriptlauncher ./hotlistsetup.sh" > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testListDetailsPopulationForHotlistV.sh"  > /dev/null 2>&1
}	

function InitFunction
{
#	shuRegTest TestListConfigsPopulationForHotlist 
	shuRegTest TestListDetailsPopulationForHotlist 
}
shuStart InitFunction
