if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+14106762273,1" $RANGERHOME/FMSData/DataRecord/Process/testPopulateCallType.cdrdata
then
	exit 1
fi

if ! grep "^2005/11/12 15:02:19,0,+14106762273,+4106052214,,1,60,2005/11/12 15:01:19,,,,3,4,4.20,8,1,3,1,,0,0,+14106762273,2" $RANGERHOME/FMSData/DataRecord/Process/testPopulateCallType.cdrdata
then
	exit 2
fi

if ! grep "^2005/11/12 15:02:19,0,+4106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,3,4,4.20,8,1,3,1,,0,0,+110226006491215,3" $RANGERHOME/FMSData/DataRecord/Process/testPopulateCallType.cdrdata
then
	exit 3
fi
if ! grep "^2005/11/12 15:02:19,0,+4106762273,+4106052214,,1,60,2005/11/12 15:01:19,,,,3,4,4.20,8,1,3,1,,0,0,+110226006491006,4" $RANGERHOME/FMSData/DataRecord/Process/testPopulateCallType.cdrdata
then
	exit 4
fi
