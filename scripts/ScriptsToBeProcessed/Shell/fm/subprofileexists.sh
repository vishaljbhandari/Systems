
if [ $# -ne 1 ] 
then
	echo "No NetworkID specIFied. Usage: subprofileexists <NetworkID>"
	exit 1	
fi

NetworkID=$1
ACTIVE_SUBSCRIBER=1
SUSPENDED_SUBSCRIBER=2
DISCONNECT_SUBSCRIBER=3

CUMULATIVE_VOICE_PROFILE=2
CUMULATIVE_VOICE_ROC_PROFILE=3
CUMULATIVE_DATA_PROFILE=4
CUMULATIVE_DATA_ROC_PROFILE=5

$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
SET SERVEROUTPUT ON ;
SET FEEDBACK ON ;
WHENEVER SQLERROR EXIT 5;
WHENEVER OSERROR EXIT 5;

DECLARE

enable_cum_voice_prof			NUMBER(10, 0) ;
enable_cum_data_prof			NUMBER(10, 0) ;
enable_cum_voice_roc_prof		NUMBER(10, 0) ;
enable_cum_data_roc_prof		NUMBER(10, 0) ;

BEGIN

	SELECT value INTO enable_cum_voice_prof FROM configurations WHERE config_key = 'AI.ENABLE_CUMULATIVE_VOICE_PROFILE' ;
	SELECT value INTO enable_cum_data_prof FROM configurations WHERE config_key = 'AI.ENABLE_CUMULATIVE_DATA_PROFILE' ;
	SELECT value INTO enable_cum_voice_roc_prof FROM configurations WHERE config_key = 'AI.ENABLE_ROC_VOICE_PROFILE' ;
	SELECT value INTO enable_cum_data_roc_prof FROM configurations WHERE config_key = 'AI.ENABLE_ROC_DATA_PROFILE' ;

	IF ( enable_cum_voice_prof = 1 )
	THEN
		PopulateSubscribers($CUMULATIVE_VOICE_PROFILE, $NetworkID, $ACTIVE_SUBSCRIBER, $SUSPENDED_SUBSCRIBER) ;
	END IF ;

	IF ( enable_cum_data_prof = 1 )
	THEN
		PopulateSubscribers($CUMULATIVE_DATA_PROFILE, $NetworkID, $ACTIVE_SUBSCRIBER, $SUSPENDED_SUBSCRIBER) ;
	END IF ;

	IF ( enable_cum_voice_roc_prof = 1 )
	THEN
		PopulateSubscribers($CUMULATIVE_VOICE_ROC_PROFILE, $NetworkID, $ACTIVE_SUBSCRIBER, $SUSPENDED_SUBSCRIBER) ;
	END IF ;

	IF ( enable_cum_data_roc_prof = 1 )
	THEN
		PopulateSubscribers($CUMULATIVE_DATA_ROC_PROFILE, $NetworkID, $ACTIVE_SUBSCRIBER, $SUSPENDED_SUBSCRIBER) ;
	END IF ;

END;
/

COMMIT;
QUIT ;
EOF

if test $? -eq 5 
then
        exitval=1
else
        exitval=0
fi
exit $exitval

