$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;

set heading off
spool testRunForParticularDayOfYearV1.dat

select 1 from dual where '1' = (select value from configurations where config_key = 'AI.CUMREC_VOICE_LAST_PROCESSED_DAY_OF_YEAR_1') ;
		
exit;
EOF

if ( [ $? -eq 5 ] || grep -i "no rows" testRunForParticularDayOfYearV1.dat )
then
	exitval=1
else
	exitval=0
fi
rm -f testRunForParticularDayOfYearV1.dat

exit $exitval

