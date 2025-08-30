# !/bin/bash

. $RANGERHOME/sbin/rangerenv.sh

HUR_RUN_DATE=`sqlplus -s /nolog << EOF
				CONNECT_TO_SQL
                set heading off ;
                set feedback off ;
                select to_char (sysdate - 1, 'dd/mm/yyyy') from dual ;
EOF`

HUR_RUN_DATE=`echo $HUR_RUN_DATE | cut -d. -f2`

$RANGERHOME/bin/scriptlauncher.sh $RANGERHOME/bin/DUInroamerHURGenerator.sh $HUR_RUN_DATE
