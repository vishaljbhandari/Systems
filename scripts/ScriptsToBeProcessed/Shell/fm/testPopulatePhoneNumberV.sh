if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+14106762273,1" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 1
fi

if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+14106762273,2" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 2
fi

if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+110226006491215,3" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 3
fi
if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+110226006491006,4" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 4
fi
if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+110226006491007,5" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 5
fi
if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+14106762273,6" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 6
fi
if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+14106762273,7" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 7
fi
if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+110226006491205,8" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 8
fi
if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+110226006491009,9" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 9
fi
if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,1,60,2005/11/12 15:01:19,,,,2,4,10.00,8,1,3,1,,0,0,+110226006491010,10" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 10
fi
if ! grep "^2005/11/12 15:02:19,0,+14106762273,+14106052214,,2,60,2005/11/12 15:01:19,,,,2,4,0.00,8,1,3,1,,0,0,+14106052214,11" $RANGERHOME/FMSData/DataRecord/Process/testPopulatePhoneNumber.cdrdata
then
	exit 11
fi

if ! grep "^Empty Phone Number : 1,11,4106762273,3,4106052214,3,10226006491005,,DUMMY,10226006491005,2260,,20051112@15:34:36,60,64910,,899690512,1,20051112@15:01:19,1,1,934,934,,,CASX" $RANGERHOME/FMSData/Datasource/CDR/Log/gbxuscdrds.log
then 
	exit 12
fi
