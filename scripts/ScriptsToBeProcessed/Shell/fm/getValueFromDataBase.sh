#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


sql_query=`echo $* | cut -d' ' -f1-$#`
TMPDIR="$WATIR_SERVER_HOME/Scripts/tmp"
if [ -d $TMPDIR ]
then
   sleep 0
else
   mkdir $TMPDIR
fi

rm $TMPDIR/sql_query.lst

if [ $DB_TYPE == "1" ]
then
sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF

set termout off ;
set heading off ;
set echo off ;

SPOOL $TMPDIR/sql_query.lst ;
$sql_query;
SPOOL OFF ;

quit ;

EOF

else

clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved." | grep -v "off" | grep -v "SPOOL" | grep -iv "SELECT" | grep -v "quit" | grep -v "SET" | grep -v ";"
set termout off ;
set heading off ;
set echo off ;
SPOOL $WATIR_SERVER_HOME/Scripts/tmp/sql_query.lst ;
$sql_query;
SPOOL OFF ;
quit ;

EOF

fi

Value_From_DataBase=`grep "[0-9][0-9]*" $TMPDIR/sql_query.lst | tr -s ' ' | tr -d ' '`

#Value_From_DataBase=`cat $TMPDIR/sql_query.lst | tr -s ' ' | tr -d ' ' | grep -v "^$"`
