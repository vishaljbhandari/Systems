#! /bin/sh

. shUnit

function TestSubscriptionFraud
{
    shuAssert "Setup" "scriptlauncher ./Tests/testsubscriptionfraudsetup.sh"  > /dev/null 2>&1
    shuAssert "Running Script" "scriptlauncher ./fraudlossavoidancesummary.sh"  > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testsubscriptionfraudV.sh" > /dev/null 2>&1
}

function TestOtherFraud
{
    shuAssert "Setup" "scriptlauncher ./Tests/testotherfraudsetup.sh"  > /dev/null 2>&1
    shuAssert "Running Script" "scriptlauncher ./fraudlossavoidancesummary.sh"  > /dev/null 2>&1
    shuAssert "Verification" "scriptlauncher ./Tests/testotherfraudV.sh" > /dev/null 2>&1
}

function InitFunction
{
    shuRegTest TestSubscriptionFraud
#    shuRegTest TestOtherFraud
}

shuStart InitFunction
