#!/bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


updateconfig()
{

if [ $# == 5 ]
then
    TABLE_NAME=$1
    SET_FIELD=$2
    VALUE=$3
    WHERE_FIELD=$4
    CONFIG_KEY=$5
elif [ $# == 4 ]
then
    TABLE_NAME="configurations"
    SET_FIELD=$1
    VALUE=$2
    WHERE_FIELD=$3
    CONFIG_KEY=$4
else
    TABLE_NAME="configurations"
    SET_FIELD="VALUE"
    WHERE_FIELD="CONFIG_KEY"
    CONFIG_KEY=$1
    shift
    VALUE=$*
fi

TMPDIR="$WATIR_SERVER_HOME/Scripts/tmp"
if [ -d $TMPDIR ]
then
        sleep 0
else
        mkdir $TMPDIR
fi
tmpsqloutput="$WATIR_SERVER_HOME/Scripts/.tmpsqloutput"

        #update configurations set value='$VALUE' where CONFIG_KEY='$CONFIG_KEY';

if [ $DB_TYPE == "1" ]
then
sqlplus -s /NOLOG << EOF > $tmpsqloutput  2>&1
    CONNECT $DB_SETUP_USER/$DB_SETUP_PASSWORD
    WHENEVER OSERROR  EXIT 6 ;
    WHENEVER SQLERROR EXIT 5 ;
	    SPOOL $TMPDIR/sqloutput

        update $TABLE_NAME set $SET_FIELD='$VALUE' where $WHERE_FIELD='$CONFIG_KEY';
        commit;
		SPOOL OFF;
        quit

EOF
else
	clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF > $tmpsqloutput 2>&1 | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved."
WHENEVER OSERROR  EXIT 6 ;
WHENEVER SQLERROR EXIT 5 ;
SPOOL $WATIR_SERVER_HOME/Scripts/tmp/sqloutput
update $TABLE_NAME set $SET_FIELD='$VALUE' where $WHERE_FIELD='$CONFIG_KEY';
commit;
SPOOL OFF;
quit;
EOF

fi

}

main()
{
        updateconfig $*
}

main $*
