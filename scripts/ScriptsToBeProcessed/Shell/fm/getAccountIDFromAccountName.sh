#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


mkdir -p $WATIR_SERVER_HOME/Scripts/tmp

account_name=$1
TMPDIR="$WATIR_SERVER_HOME/Scripts/tmp"
tmpsqlOP=.tmpsqlOP

>$TMPDIR/account_id.lst
. ~/.bash_profile

if [ $DB_TYPE == "1" ]
then

sqlplus -s /NOLOG << EOF > $tmpsqlOP  2>&1
    CONNECT $DB_SETUP_USER/$DB_SETUP_PASSWORD
    SET HEADING OFF ;
    WHENEVER OSERROR  EXIT 6 ;
    WHENEVER SQLERROR EXIT 5 ;
    SPOOL $TMPDIR/account_id.lst
    select id from account where account_name='$account_name' ;
    SPOOL OFF ;
EOF

else

clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF > $tmpsqlOP  2>&1 | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."
SET HEADING OFF ;
WHENEVER OSERROR  EXIT 6 ;
WHENEVER SQLERROR EXIT 5 ;
SPOOL $WATIR_SERVER_HOME/Scripts/tmp/account_id.lst ;
select id from account where account_name='$account_name' ;
SPOOL OFF ;
EOF

fi

account_id=`cat $TMPDIR/account_id.lst | head -2 | tail -1 | tr -s " " | cut -d " " -f2`
echo "$account_id"
