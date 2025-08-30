#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


mkdir -p $WATIR_SERVER_HOME/Scripts/tmp

TMPDIR="$WATIR_SERVER_HOME/Scripts/tmp"
tmpsqlOP=.tmpsqlOP

>$TMPDIR/description.lst
. ~/.bash_profile
. ~/.bashrc
description=$1
description=${description//WHITESPACE/\ }

if [ $DB_TYPE == "1" ]
then

sqlplus -s /NOLOG << EOF > $tmpsqlOP  2>&1
    CONNECT $DB_SETUP_USER/$DB_SETUP_PASSWORD
    SET HEADING OFF ;
    WHENEVER OSERROR  EXIT 6 ;
    WHENEVER SQLERROR EXIT 5 ;
    SPOOL $TMPDIR/description.lst
    SELECT SOURCE_ID FROM AUDIT_LOG_EVENT_CODES where DESCRIPTION = '$description' ;
    SPOOL OFF ;
EOF

else
clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF > $tmpsqlOP  2>&1 | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."
SET HEADING OFF ;
WHENEVER OSERROR  EXIT 6 ;
WHENEVER SQLERROR EXIT 5 ;
SPOOL $WATIR_SERVER_HOME/Scripts/tmp/description.lst
SELECT SOURCE_ID FROM AUDIT_LOG_EVENT_CODES where DESCRIPTION = '$description' ;
SPOOL OFF ;
EOF

fi

source_id=`cat $TMPDIR/description.lst | head -2 | tail -1 | tr -d " "| tr -s " "`
echo "$source_id"
