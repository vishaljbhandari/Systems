#! /bin/sh

. shUnit

function TestIpdrSummaryWithoutPartition
{
	NETWORK_ID=1025
    SUMMARY_DATE=11-01-2007	
	shuAssert "Setup" "scriptlauncher ./Tests/testipdrsummarywithoutpartitionsetup.sh" > /dev/null 2>&1
	shuAssert "Running script" "scriptlauncher ./ipdrsessionsummary.sh $NETWORK_ID $SUMMARY_DATE"  > /dev/null 2>&1
	shuAssert "Verification" "scriptlauncher ./Tests/testipdrsummarywithoutpartitionV.sh" > /dev/null 2>&1
}

function TestIpdrSummaryWithPartition
{
	NETWORK_ID=1025
    SUMMARY_DATE='11-01-2007'	
	shuAssert "Setup" "scriptlauncher ./Tests/testipdrsummarywithpartitionsetup.sh"  > /dev/null 2>&1
	shuAssert "Running script" "scriptlauncher ./ipdrsessionsummary.sh $NETWORK_ID $SUMMARY_DATE 11 1 Y"  > /dev/null 2>&1
	shuAssert "Verification" "scriptlauncher ./Tests/testipdrsummarywithpartitionV.sh" > /dev/null 2>&1
}

function InitFunction
{
	shuRegTest TestIpdrSummaryWithoutPartition
	shuRegTest TestIpdrSummaryWithPartition
}

shuStart InitFunction

