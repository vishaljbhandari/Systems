
ls $COMMON_MOUNT_POINT/FMSData/AICumulativeVoiceData/AICRG_VOICE_D10_N0_P0.txt
if test $? -eq 0
then
	exit 1
fi

ls $COMMON_MOUNT_POINT/FMSData/AICumulativeVoiceData/success/AICRG_VOICE_D10_N0_P0.txt
if test $? -eq 0
then
	exit 1
fi

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;

set heading off
spool testDeleteEmptyOutputFileV.dat

select 1 from configurations where config_key = 'AI.CUMREC_VOICE_LAST_PROCESSED_DAY_OF_YEAR_1' and value = '10' ;

EOF

if ( [ $? -eq 5 ] || grep -i "no rows" testDeleteEmptyOutputFileV.dat )
then
	exitval=1
else
	exitval=0
fi

rm -f testDeleteEmptyOutputFileV.dat

exit $exitval 
