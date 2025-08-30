#!/bin/bash
. ~/.bashrc

echo "Enter the absolute path of a directory for storing the generated Data Records:"
read DataRecordsDir
if [ "x$DataRecordsDir" == "x" ];then
     DataRecordsDir=`pwd`
fi
echo $DataRecordsDir
export DataRecordsDir=$DataRecordsDir

#echo "Bash version ${BASH_VERSION}..."

#----------Test Data Generation for Rule "Count_CDR_GPRS_Rech_LOG" and test case "TS-ROCFM-001"---------
for i in {1..3}
do
bash generateTestData.sh "PhoneNumber=+17034244411" "CallerNumber=+17034244411" "Value=$((10+($i*5)))" 'OutPutFile=Count_CDR_GPRS_Rech_LOG.txt' 'DS=CDR' 'lookback=1' 'Records=1'
bash generateTestData.sh "PhoneNumber=+92098554411" "CallerNumber=+92098554411" "Value=$((10+($i*5)))" 'OutPutFile=Count_CDR_GPRS_Rech_LOG.txt' 'DS=CDR' 'lookback=1' 'Records=1'
bash generateTestData.sh "PhoneNumber=+7204201101" "CallerNumber=+7204201101" "Value=$((10+($i*5)))" 'OutPutFile=Count_CDR_GPRS_Rech_LOG.txt' 'DS=CDR' 'lookback=1' 'Records=1'
done

#----------Test Data Generation for Rule "Per Call Value Rule" and test case "TS-ROCFM-002"---------------------
for i in {1..3}
do
bash generateTestData.sh "PhoneNumber=+1720420111$i" "CallerNumber=+1720420111$i" "Value=$((199+$i))" 'OutPutFile=PerCallValue.txt' 'DS=CDR' 'lookback=0' 'Records=1'
done
#----------Test Data Generation for Rule "Per Call Duration Rule" and test case "TS-ROCFM-003"---------------------

bash generateTestData.sh "PhoneNumber=+17204201114" "CallerNumber=+17204201114" "Value=150" "Duration=5500" 'OutPutFile=PerCallDuration.txt' 'DS=CDR' 'lookback=0' 'Records=1'

#----------Test Data Generation for Rule "CallCollisionRule" and test case "TS-ROCFM-004"---------------------

for i in {1..3}
do
bash generateTestData.sh "PhoneNumber=+17204201115" "CallerNumber=+17204201115" "Duration=100" "EquipmentID=$((1111111111*$i))" 'OutPutFile=CallCollision.txt' 'DS=CDR' 'lookback=1' 'Records=1'
done
#----------Test Data Generation for Rule "AdjustmentLogCallCountRule" and test case "TS-ROCFM-005" ---------------------

./generateTestData.sh 'InternalUserID=01' 'FirstName=Dola' 'EmployeeCode=950' 'OutPutFile=InternalUsers.txt' 'DS=Internal User' 'lookback=0' 'Records=1'

for i in {1..5}
do
bash generateTestData.sh 'InternalUserID=01' 'AdjustmentDescription=Postpaid subscribers amount decreased' 'Amount=10000.0' 'EmployeeCode=950' 'OutPutFile=AdjustmentLogCallCountRule.txt' 'DS=Adjustment Logs' 'lookback=1' 'Records=1'
done
#----------Test Data Generation for Rule "OddValueTopUp" and test case "TS-ROCFM-006" --------------------------

bash generateTestData.sh "PhoneNumber=+17204201116" "Amount=500" "Network=1025" 'OutPutFile=OddValtopUp.txt' 'DS=Recharge Log' 'lookback=0' 'Records=1'

#-----------Test Data Generation for Rule "SuspiciousRecharge Rule" and test case "TS-ROCFM-007" ----------------------

for i in {1..3}
do
bash generateTestData.sh "PhoneNumber=+17204201117" "Amount=25$i" "Network=1025" 'OutPutFile=SuspiciousRecharge.txt' 'DS=Recharge Log' 'lookback=0' 'Records=1'
done

#-----------Test Data Generation for Rule "Rule_AccountNameCumulativeDuration_CurrentMonth" and test case "TS-ROCFM-008" -----------
bash generateTestData.sh "PhoneNumber=+17204201118" "CallerNumber=+17204201118" "Network=1025" "Duration=9630" 'OutPutFile=AccountCumulativeDurationAcc.txt' 'DS=CDR' 'lookback=1' 'Records=3'

#----------Test Data Generation for Rule "LocalDistinctDestinationCount" and test case "TS-ROCFM-009" ----------------
for i in {1..3}
do
bash generateTestData.sh "PhoneNumber=+17204201119" "CallerNumber=+17204201119" "Duration=$((i))30" "CalledNumber=+919100000$i" 'OutPutFile=DistinctDestination.txt' 'DS=CDR' 'lookback=1' 'Records=1'
done

#----------Test Data Generation for Rule "volumeviolation" and test case "TS-ROCFM-010" ---------------------
for i in {6..7}
do
bash generateTestData.sh "PhoneNumber=+1720420112$i" "UploadDataVolume=250" "DownloadDataVolume=140" "ChargingID=25" "Network=1025" 'OutPutFile=VolumeViolation.txt' 'DS=GPRS' 'lookback=1' 'Records=1'
done

#---------Test Data Generation for Rule "InProgressValue_PhoneNumber" and test case "TS-ROCFM-011" -------------
for i in {1..4}
do
bash generateTestData.sh "PhoneNumber=+17204201120" "CallerNumber=+17204201120" "Value=3$i" "IsComplete=0" "Co-RelatedField=1" 'OutPutFile=Inprogress.txt' 'DS=CDR' 'lookback=0' 'Records=1'
done
bash generateTestData.sh "PhoneNumber=+17204201120" "CallerNumber=+17204201120" "Value=32" "IsComplete=1" "Co-RelatedField=1" 'OutPutFile=Inprogress.txt' 'DS=CDR' 'lookback=0' 'Records=1'

#---------Test Data Generation for Rule "Distinct Count on IMSI" and test case "TS-ROCFM-012" ---------------
for i in {1..4}
do
bash generateTestData.sh "PhoneNumber=+17204201121" "CallerNumber=+17204201126" "Value=37" "IMSI\/ESNNumber=233$i" 'OutPutFile=DistIMSICount.txt' 'DS=CDR' 'lookback=1' 'Records=1'
done


#----------Test Data Generation for Rule "ServiceViolation" and test case "TS-ROCFM-013" ------------------
bash generateTestData.sh "PhoneNumber=+17204201129" "CallerNumber=+17204201129" "Value=398" "Service=4" "RecordType=1" 'OutPutFile=ServiceViolation.txt' 'DS=GPRS' 'lookback=1' 'Records=1'
bash generateTestData.sh "PhoneNumber=+17204201129" "CallerNumber=+17204201129" "Value=398" "Service=4" "RecordType=1" 'OutPutFile=ServiceViolation.txt' 'DS=GPRS' 'lookback=1' 'Records=1'

#--------Test Data Generation for Rule "InvalidSubscriber" and test case "TS-ROCFM-015" -------------------
bash generateTestData.sh "PhoneNumber=+17204203129" "CallerNumber=+17204203129" "Value=300" "CDRType=7" 'OutPutFile=InvalidSubscriber.txt' 'DS=CDR' 'lookback=3' 'Records=1'
bash generateTestData.sh "PhoneNumber=+17204203129" "CallerNumber=+17204203129" "Value=300" "CDRType=7" 'OutPutFile=InvalidSubscriber.txt' 'DS=CDR' 'lookback=2' 'Records=1'
bash generateTestData.sh "PhoneNumber=+17204203129" "CallerNumber=+17204203129" "Value=300" "CDRType=7" 'OutPutFile=InvalidSubscriber.txt' 'DS=CDR' 'lookback=1' 'Records=1'

#---------Test Data Generation for Rule "FPRule"  and test case "TS-ROCFM-016"----------------------

bash generateTestData.sh 'PhoneNumber=+17204201122' 'CallerNumber=+17204201122'  'CalledNumber=+17204201183'  'CallType=1'  'VPMN=27970290' 'OutPutFile=FPRule.txt' 'DS=CDR' 'lookback=1' 'Records=5'
bash generateTestData.sh 'PhoneNumber=+17204201122' 'CallerNumber=+17204201122'  'CalledNumber=+91981200576'  'CallType=2'  'VPMN=27970290' 'OutPutFile=FPRule.txt' 'DS=CDR' 'lookback=1' 'Records=8'
bash generateTestData.sh 'PhoneNumber=+17204201122' 'CallerNumber=+17204201122'  'CalledNumber=+44196781240'  'CallType=3'  'VPMN=27970291' 'OutPutFile=FPRule.txt' 'DS=CDR' 'lookback=1' 'Records=1'
bash generateTestData.sh 'PhoneNumber=+17204201128' 'CallerNumber=+17204201128'  'CalledNumber=+17204201183'  'CallType=1'  'VPMN=27970290' 'OutPutFile=FPRule.txt' 'DS=CDR' 'lookback=1' 'Records=5'
bash generateTestData.sh 'PhoneNumber=+17204201128' 'CallerNumber=+17204201128'  'CalledNumber=+91981200576'  'CallType=2'  'VPMN=27970291' 'OutPutFile=FPRule.txt' 'DS=CDR' 'lookback=1' 'Records=8'
bash generateTestData.sh 'PhoneNumber=+17204201128' 'CallerNumber=+17204201128'  'CalledNumber=+44196781240'  'CallType=3'  'VPMN=27970292' 'OutPutFile=FPRule.txt' 'DS=CDR' 'lookback=1' 'Records=1'

#-------------Test Data Generation for Rule "IMEI statistical Rule" and test case "TS-ROCFM-017" --------------
bash generateTestData.sh "PhoneNumber=+17204201123" "CallerNumber=+17204201123" "EquipmentID=55343" 'OutPutFile=IMEIStatisticalCDR.txt' 'DS=CDR' 'lookback=1' 'Records=5'
bash generateTestData.sh "PhoneNumber=+17204201123" "CallerNumber=+17204201123" "EquipmentID=55343" 'OutPutFile=IMEIStatisticalGPRS.txt' 'DS=GPRS'  'lookback=1' 'Records=20'
bash generateTestData.sh "PhoneNumber=+17204201123" "CallerNumber=+17204201123" "EquipmentID=55343" 'OutPutFile=IMEIStatisticalCDR.txt' 'DS=CDR' 'lookback=1' 'Records=25'

#--------Test Data Generation for Rule "IPDR Cumulative Data vol Rule" and test case "TS-ROCFM-018" ----------------
bash generateTestData.sh "PhoneNumber=+17204201123" "UserName=+17204201123" "UploadDataVolume=23" "DownloadDataVolume=235" "Network=1025" 'OutPutFile=IPDRCumDatavolume.txt' 'DS=IPDR' 'lookback=1' 'Records=1'
bash generateTestData.sh "PhoneNumber=+17204201123" "UserName=+17204201123" "UploadDataVolume=23" "DownloadDataVolume=235" "Network=1025" 'OutPutFile=IPDRCumDatavolume.txt' 'DS=IPDR' 'lookback=1' 'Records=1'
bash generateTestData.sh "PhoneNumber=+17204201123" "UserName=+17204201123" "UploadDataVolume=23" "DownloadDataVolume=235" "Network=1025" 'OutPutFile=IPDRCumDatavolume.txt' 'DS=IPDR' 'lookback=1' 'Records=1'


