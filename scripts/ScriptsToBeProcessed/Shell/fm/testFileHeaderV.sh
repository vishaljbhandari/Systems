if ! grep "^RecordType:1" $RANGERHOME/FMSData/DataRecord/Process/testpopulatesubscriberid.cdrdata
then
	exit 1
fi

if ! grep "^2005/11/12 18:56:42,0,+14106762273,+14106052214,,1,14123,2005/11/12 15:01:19,,,,2,1810,2360.00,3,1,3,1,,0,0,+110226006491005,1,3,+110226006491005,,10226006491005,2260,,2005/11/12 15:34:36,64910,,899690502,1,1,934,934,,,CASX,Dedicated" $RANGERHOME/FMSData/DataRecord/Process/testpopulatesubscriberid.cdrdata
then
	exit 2
fi

if ! grep "^2005/11/12 18:56:42,0,+14106762273,+18883966990,,2,14117,2005/11/12 15:01:25,,,,2,1,0.00,7,1,3,1,,0,0,+18883966990,1,3,+199005,,99005,933,,2005/11/12 15:34:39,21985,,,1,1,2261,2261,,8883966990,,Calling Card" $RANGERHOME/FMSData/DataRecord/Process/testpopulatesubscriberid.cdrdata
then 
	exit 3
fi

if ! grep "^2005/11/12 18:56:42,0,+14106762276,+14106052214,,1,14123,2005/11/12 15:01:19,,,,2,4,2360.00,8,1,3,1,,0,0,+14106762276,1,3,+110226006491005,,10226006491005,2260,,2005/11/12 15:34:36,64910,,899690502,1,1,934,934,,,CASX,VOIP 1+" $RANGERHOME/FMSData/DataRecord/Process/testpopulatesubscriberid.cdrdata
then
	exit 4
fi

