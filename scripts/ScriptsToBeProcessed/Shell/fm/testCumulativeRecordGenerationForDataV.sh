
grep "1025,+919820535200,1,1,30.00,300.00,230,1,+91,1,6,12789,0" $COMMON_MOUNT_POINT/FMSData/AICumulativeGPRSData/Process/AICRG_DATA_D10_N1_P0_S0.txt
if test $? -ne 0
then
	exit 1
fi

grep "1025,+919820535200,1,1,30.00,1.00,0,1,+,2,7,12790,0" $COMMON_MOUNT_POINT/FMSData/AICumulativeGPRSData/Process/AICRG_DATA_D10_N1_P0_S0.txt
if test $? -ne 0
then
	exit 1
fi

grep "1025,+919820535200,2,1,60.00,600.00,460,1,+91,1,9,12792,1" $COMMON_MOUNT_POINT/FMSData/AICumulativeGPRSData/Process/AICRG_DATA_D10_N1_P0_S0.txt
if test $? -ne 0
then
	exit 1
fi

grep "1027,+919820535202,1,1,30.00,300.00,230,1,+,1,8,12791,0" $COMMON_MOUNT_POINT/FMSData/AICumulativeGPRSData/Process/AICRG_DATA_D10_N1_P0_S0.txt
if test $? -ne 0
then
	exit 1
fi

grep "1027,+919820535202,1,2,30.00,300.00,230,1,+,1,9,12792,1" $COMMON_MOUNT_POINT/FMSData/AICumulativeGPRSData/Process/AICRG_DATA_D10_N1_P0_S0.txt
if test $? -ne 0
then
	exit 1
fi

grep "1028,+919820535203,1,1,30.00,300.00,230,1,+91,1,6,12789,1" $COMMON_MOUNT_POINT/FMSData/AICumulativeGPRSData/Process/AICRG_DATA_D10_N1_P0_S0.txt
if test $? -ne 0
then
	exit 1
fi

NoOfRecords=`cat $COMMON_MOUNT_POINT/FMSData/AICumulativeGPRSData/Process/AICRG_DATA_D10_N1_P0_S0.txt | wc -l`
if [ "$NoOfRecords" -ne "7" ]
then
	exit 1
fi

ls $COMMON_MOUNT_POINT/FMSData/AICumulativeGPRSData/Process/success/AICRG_DATA_D10_N1_P0_S0.txt
if test $? -ne 0
then
	exit 1
fi

exit 0

