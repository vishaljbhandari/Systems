
grep "1025,+919820535200,1,1,1,300.00,230,1,1,6,12789,1" $COMMON_MOUNT_POINT/FMSData/AICumulativeVoiceData/Process/AICRG_VOICE_D7_N1_P0_S0.txt
if test $? -ne 0
then
	exit 1
fi

grep "1028,+919820535203,1,1,1,300.00,230,1,1,6,12789,1" $COMMON_MOUNT_POINT/FMSData/AICumulativeVoiceData/Process/AICRG_VOICE_D7_N1_P0_S0.txt
if test $? -ne 0
then
	exit 1
fi

NoOfRecords=`cat $COMMON_MOUNT_POINT/FMSData/AICumulativeVoiceData/Process/AICRG_VOICE_D7_N1_P0_S0.txt | wc -l`
if [ "$NoOfRecords" -ne "3" ]
then
	exit 1
fi

ls $COMMON_MOUNT_POINT/FMSData/AICumulativeVoiceData/Process/success/AICRG_VOICE_D7_N1_P0_S0.txt
if test $? -ne 0
then
	exit 1
fi

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

set heading off
spool testRunForParticularDayOfYearV.dat

select 1 from dual where '7' = (select value from configurations where config_key = 'AI.CUMREC_VOICE_LAST_PROCESSED_DAY_OF_YEAR_1') ;
		
exit;
EOF

if ( [ $? -eq 5 ] || grep -i "no rows" testRunForParticularDayOfYearV.dat )
then
	exitval=1
else
	exitval=0
fi
rm -f testRunForParticularDayOfYearV1.dat

exit $exitval

