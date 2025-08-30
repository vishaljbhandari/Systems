#! /bin/sh

. shUnit

function TestSubscriberGroupSummary
{
	shuAssert "Setup" "scriptlauncher  ./Tests/testsubscribergroupsummarysetup.sh"   > /dev/null 2>&1
	shuAssert "Running script" "scriptlauncher  ./subscriber_group_summary.sh  "     > /dev/null 2>&1
	shuAssert "Verification" "scriptlauncher ./Tests/testsubscribergroupsummaryV.sh"   > /dev/null 2>&1
}

function InitFunction
{
	shuRegTest TestSubscriberGroupSummary
}

shuStart InitFunction

