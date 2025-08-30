#! /bin/bash
                                                                                                      
#/*******************************************************************************                                 
#*  Copyright (c) Subex Limited 2014. All rights reserved.                      *                                 
#*  The copyright to the computer program(s) herein is the property of Subex    *                                 
#*  Limited. The program(s) may be used and/or copied with the written          *                                 
#*  permission from Subex Limited or in accordance with the terms and           *                                 
#*  conditions stipulated in the agreement/contract under which the program(s)  *                                 
#*  have been supplied.                                                         *                                 
#********************************************************************************/ 

. shUnit

SampleData="TestData/vpmn_loader_all_valid.data"
# testing with Bulky Data
#SampleData="TestData/vpmn_loader_all_valid.bulky.data"
function testLoadSuccess
{
    shuAssert "Setup" "scriptlauncher TestScripts/VPMNLoader_setup.sh" >/dev/null 2>&1
    shuAssert "Running Script" "scriptlauncher loadVPMNData.sh LOAD $SampleData" >/dev/null 2>&1
    shuAssert "Verification" "scriptlauncher TestScripts/VPMNLoader_verify.sh $SampleData"  >/dev/null 2>&1
}


function testAppendSuccess
{
    shuAssert "Setup" "scriptlauncher TestScripts/VPMNLoader_setup.sh" >/dev/null 2>&1
    shuAssert "Running LOAD Script" "scriptlauncher loadVPMNData.sh LOAD $SampleData" >/dev/null 2>&1
    shuAssert "Running APPED Script" "scriptlauncher loadVPMNData.sh APPEND $SampleData" >/dev/null 2>&1
    shuAssert "Verification" "scriptlauncher TestScripts/VPMNLoader_verify_for_append.sh $SampleData"  >/dev/null 2>&1
}



function InitFunction
{
	shuRegTest testLoadSuccess
	shuRegTest testAppendSuccess

}

shuStart InitFunction

