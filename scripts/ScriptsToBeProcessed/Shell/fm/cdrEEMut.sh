#! /bin/bash

source $PWD/shUnit
source $PWD/utilities

datastreamDirectoryName="CDR"
datasourceDirectoryName="EEM"

sleepTime=30

function ProcessPostpaidSubscriber
{
        $ATPATH/subscriber_cleanup.sh 
	$ATPATH/subscribers_insert.sql
        echo "Subscribers processed"
}

function RunRater
{
        $RANGERHOME/sbin/tmrater &  > /dev/null 2>&1
	sleep 5
        for i in `ps -u$LOGNAME | grep tmrat|awk {'print $1'}`
         do
           kill -9 $i
         done
}

function testCalledNumberManditoryFormatCheck
{	
	removeFiles $datastreamDirectoryName $datasourceDirectoryName 
	startDatasources 
	sleep $sleepTime
	copyFile $datastreamDirectoryName $datasourceDirectoryName "cdrEEMCalledNumberManditoryFormatCheck.txt"
	sleep $sleepTime
	shuAssert "Verifaction " "compare cdrEEMCalledNumberManditoryFormatCheckV.txt DataRecord Process cdrEEMCalledNumberManditoryFormatCheck.txt"  
	stopDatasources 
}

function testCalledNumberUNFCheck
{
        echo "EEMCDR : Test Case to verify Called Number UNF Conversion  in rff"
        removeFiles $datastreamDirectoryName $datasourceDirectoryName
        startDatasources
        sleep $sleepTime
        copyFile $datastreamDirectoryName $datasourceDirectoryName "cdrEEMCalledNumberUNFCheck.txt"
        sleep $sleepTime
        shuAssert "Verifaction " "compare cdrEEMCalledNumberUNFCheckV.txt DataRecord Process cdrEEMCalledNumberUNFCheck.txt"
        stopDatasources  
        
}

function testCallerNumberManditoryFormatCheck 
{
        echo "EEMCDR : Test Case to verify Caller Number Manditory check and Format check   in rff"
        removeFiles $datastreamDirectoryName $datasourceDirectoryName
        startDatasources
        sleep $sleepTime
        copyFile $datastreamDirectoryName $datasourceDirectoryName "cdrEEMCallerNumberManditoryFormatCheck.txt"
        sleep $sleepTime
        shuAssert "Verifaction " "compare cdrEEMCallerNumberManditoryFormatCheckV.txt DataRecord Process cdrEEMCallerNumberManditoryFormatCheck.txt"
        stopDatasources

}

function testCallerNumberUNFCheck 
{
        echo "EEMCDR : Test Case to verify Caller Number UNF Conversion field in rff"
        removeFiles $datastreamDirectoryName $datasourceDirectoryName
        startDatasources
        sleep $sleepTime
        copyFile $datastreamDirectoryName $datasourceDirectoryName "cdrEEMCallerNumberUNFCheck.txt"
        sleep $sleepTime
        shuAssert "Verifaction " "compare cdrEEMCallerNumberUNFCheckV.txt DataRecord Process cdrEEMCallerNumberUNFCheck.txt"
        stopDatasources

}

function testCallTypeCheck 
{
        echo "EEMCDR : Test Case to verify CallType Validation  in rff"
        removeFiles $datastreamDirectoryName $datasourceDirectoryName
        startDatasources
        sleep $sleepTime
        copyFile $datastreamDirectoryName $datasourceDirectoryName "cdrEEMCallTypeCheck.txt"
        sleep $sleepTime
        shuAssert "Verifaction " "compare cdrEEMCallTypeCheckV.txt DataRecord Process cdrEEMCallTypeCheck.txt"
        stopDatasources

}

function testDurationCheck
{
        echo "EEMCDR : Test Case to check  Duration Field manditory condition  in rff"
        removeFiles $datastreamDirectoryName $datasourceDirectoryName
        startDatasources
        sleep $sleepTime
        copyFile $datastreamDirectoryName $datasourceDirectoryName "cdrEEMDurationCheck.txt"
        sleep $sleepTime
        shuAssert "Verifaction " "compare cdrEEMDurationCheckV.txt DataRecord Process cdrEEMDurationCheck.txt"
        stopDatasources


}


function testPhoneNumberManditoryFormatCheck
{
        echo "EEMCDR : Test Case to verify Phone Number Manditory check and Format check in rff"
        removeFiles $datastreamDirectoryName $datasourceDirectoryName
        startDatasources
        sleep $sleepTime
        copyFile $datastreamDirectoryName $datasourceDirectoryName "cdrEEMPhoneNumberManditoryFormatCheck.txt"
        sleep $sleepTime
        shuAssert "Verifaction " "compare cdrEEMPhoneNumberManditoryFormatCheckV.txt DataRecord Process cdrEEMPhoneNumberManditoryFormatCheck.txt"
        stopDatasources
 
}

function testPhoneNumberUNFCheck
{
        echo "EEMCDR : Test Case to verify Phone Number UNF Conversion Logic  in rff"
        removeFiles $datastreamDirectoryName $datasourceDirectoryName
        startDatasources
        sleep $sleepTime
        copyFile $datastreamDirectoryName $datasourceDirectoryName "cdrEEMPhoneNumberUNFCheck.txt"
        sleep $sleepTime
        shuAssert "Verifaction " "compare cdrEEMPhoneNumberUNFCheckV.txt DataRecord Process cdrEEMPhoneNumberUNFCheck.txt"
        stopDatasources
}


function testTimestampManditoryFormatCheck
{
        echo "EEMCDR : Test Case to verify  Timestamp  Manditory check and Format check in rff "
        #ProcessPostpaidSubscriber
        #RunRater
        removeFiles $datastreamDirectoryName $datasourceDirectoryName
        startDatasources
        sleep $sleepTime
        copyFile $datastreamDirectoryName $datasourceDirectoryName "cdrEEMTimestampManditoryFormatCheck.txt"
        sleep $sleepTime
        shuAssert "Verifaction " "compare cdrEEMTimestampManditoryFormatCheckV.txt DataRecord Process cdrEEMTimestampManditoryFormatCheck.txt"
        stopDatasources
}


function InitFunction
{
	shuAssert "Inserting duplicate file handling" "sqlplus -s $DB_USER_SPARK/$DB_PASSWORD_SPARK @duplicate.sql" > /dev/null 2>&1
        shuRegTest testCalledNumberManditoryFormatCheck
        shuRegTest testCalledNumberUNFCheck
        shuRegTest testCallerNumberManditoryFormatCheck
        shuRegTest testCallerNumberUNFCheck
        shuRegTest testCallTypeCheck
        shuRegTest testDurationCheck
        shuRegTest testPhoneNumberManditoryFormatCheck
        shuRegTest testPhoneNumberUNFCheck
        shuRegTest testTimestampManditoryFormatCheck
}

shuStart InitFunction
