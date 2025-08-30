#! /bin/sh

NON_FRAUD_SUBSCRIBER=0
FRAUD_SUBSCRIBER=1
PROFILE_PROCESS=0
BLACKLIST_PROFILE_PROCESS=1


. shUnit


function TestLifestyleProfileGenerationBlackListBulk
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationblacklistsetup.sh" 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorbulk.rb 1024 $FRAUD_SUBSCRIBER $BLACKLIST_PROFILE_PROCESS" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationblacklistV.sh" 
}> /dev/null 2>&1

function TestLifestyleProfileGenerationSubscriberlessBulk
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationsubscriberlesssetup.sh" 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorbulk.rb 1024 $FRAUD_SUBSCRIBER $BLACKLIST_PROFILE_PROCESS" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationsubscriberlessV.sh" 
} > /dev/null 2>&1 

function TestLifestyleProfileGenerationSubscriberlessBulkEmptyCellsite
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationsubscriberlesssetup1.sh" 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorbulk.rb 1024 $FRAUD_SUBSCRIBER $BLACKLIST_PROFILE_PROCESS" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationsubscriberlessV1.sh" 
} > /dev/null 2>&1 

function TestLifestyleProfileGenerationActiveListMultipleSubscribersBulk
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationmultipleactivesubscriberssetup.sh" > /dev/null 2>&1 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorbulk.rb 1024 $NON_FRAUD_SUBSCRIBER $PROFILE_PROCESS" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationmultipleactivesubscriberV.sh" > /dev/null 2>&1 
} > /dev/null 2>&1 

function TestLifestyleProfileGenerationActiveListSubscribersBulkForBlakListProfileProcess
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationmultipleactivesubscriberssetup.sh" > /dev/null 2>&1 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorbulk.rb 1024 $NON_FRAUD_SUBSCRIBER $BLACKLIST_PROFILE_PROCESS" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationactivesubscriberbulkforblacklistprofileprocessV.sh" > /dev/null 2>&1 
} > /dev/null 2>&1 

function TestBlackListROCProfileForBulkProcessing
{
	shuAssert "Setup" "scriptlauncher ./Tests/blacklistrocprofileforbulkprocessing_setup.sh" 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorbulk.rb 1024 $FRAUD_SUBSCRIBER $BLACKLIST_PROFILE_PROCESS" 
	shuAssert "Verification" "scriptlauncher ./Tests/blacklistrocprofileforbulkprocessing_verification.sh" 
} > /dev/null 2>&1

function TestLifestyleProfileGenerationBlackListBulkWithMixedSubscribersPresent 
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationblacklistbulkwithmixedsubscriberspresent_setup.sh"	
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorbulk.rb 1024 $FRAUD_SUBSCRIBER $BLACKLIST_PROFILE_PROCESS" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationblacklistbulkwithmixedsubscriberspresent_verification.sh"
} > /dev/null 2>&1

function TestLifestyleProfileGenerationBlackListForSingleSubscriber
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationblacklistsinglesubscribersetup.sh" 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorsinglesubscriber.rb 10253 $FRAUD_SUBSCRIBER +919845200005" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationblacklistsinglesubscriberV.sh" 
} > /dev/null 2>&1 

function TestLifestyleProfileGenerationBlackListForSingleSubscriberWithoutProfileOption
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationblacklistsinglesubscriberwithoutprofileoptionsetup.sh" 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorsinglesubscriber.rb 10253 $FRAUD_SUBSCRIBER +919845200005" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationblacklistsinglesubscriberwithouprofileoptionV.sh" 
} > /dev/null 2>&1 

function TestLifestyleProfileGenerationActiveListSingleSubscriber
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationactivesubscribersetup.sh" 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorsinglesubscriber.rb 5200 $NON_FRAUD_SUBSCRIBER +919845200005" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationactivesubscriberV.sh" 
} > /dev/null 2>&1 

function TestLifestyleProfileGenerationActiveListSingleSubscriberWithMoreCDR
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationactivesubscribersetupwithmorecdr.sh" 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorsinglesubscriber.rb 5200 $NON_FRAUD_SUBSCRIBER +919845200005" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationactivesubscriberwithmorecdrV.sh" 
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationactivesubscribersetupwithmorecdr1.sh" 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorsinglesubscriber.rb 5200 $NON_FRAUD_SUBSCRIBER +919845200005" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationactivesubscriberwithmorecdr1V.sh" 
} > /dev/null 2>&1 

function TestLifestyleProfileGenerationActiveListSingleSubscriberOnlyGPRS
{
	shuAssert "Setup" "scriptlauncher ./Tests/lifestyleprofilegenerationactivesubscriberonlygprssetup.sh" 
	shuAssert "Running script" "ruby RubyScripts/lifestyleprofilegeneratorsinglesubscriber.rb 1025 $NON_FRAUD_SUBSCRIBER +919812400401" 
	shuAssert "Verification" "scriptlauncher ./Tests/lifestyleprofilegenerationactivesubscriberonlygprsV.sh" 
} > /dev/null 2>&1 



function InitFunction
{
	export TEST_RUN="1"

	shuRegTest TestLifestyleProfileGenerationBlackListBulk
	shuRegTest TestLifestyleProfileGenerationSubscriberlessBulk
	shuRegTest TestLifestyleProfileGenerationSubscriberlessBulkEmptyCellsite
	shuRegTest TestLifestyleProfileGenerationActiveListMultipleSubscribersBulk
	shuRegTest TestLifestyleProfileGenerationActiveListSubscribersBulkForBlakListProfileProcess
	shuRegTest TestBlackListROCProfileForBulkProcessing
	shuRegTest TestLifestyleProfileGenerationBlackListBulkWithMixedSubscribersPresent
	shuRegTest TestLifestyleProfileGenerationBlackListForSingleSubscriber
	shuRegTest TestLifestyleProfileGenerationBlackListForSingleSubscriberWithoutProfileOption
	shuRegTest TestLifestyleProfileGenerationActiveListSingleSubscriber
	shuRegTest TestLifestyleProfileGenerationActiveListSingleSubscriberWithMoreCDR
	shuRegTest TestLifestyleProfileGenerationActiveListSingleSubscriberOnlyGPRS
}

shuStart InitFunction
