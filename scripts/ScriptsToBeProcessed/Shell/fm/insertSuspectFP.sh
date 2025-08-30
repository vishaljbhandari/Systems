#!/bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


insertSuspectFP()
{

if [ $# == 3 ]
then
    INSERT_TABLE_NAME="fp_suspect_profiles_"$1
    SELECT_TABLE_NAME="fp_profiles_"$1"_0_"$2
    PROFILED_ENTITY_ID=$3
else
    "echo Invalid Parameters. Usage: (16 or 221) CurrentRunIdentifier ProfiledEntityID"
    exit 0
fi

TMPDIR="$WATIR_SERVER_HOME/Scripts/tmp"
if [ -d $TMPDIR ]
then
        sleep 0
else
        mkdir $TMPDIR
fi
tmpsqloutput="$WATIR_SERVER_HOME/Scripts/.tmpsqloutput"

sqlplus -s /NOLOG << EOF > $tmpsqloutput  2>&1
    CONNECT $DB_SETUP_USER/$DB_SETUP_PASSWORD
    WHENEVER OSERROR  EXIT 6 ;
    WHENEVER SQLERROR EXIT 5 ;
	    SPOOL $TMPDIR/sqloutput
        insert into $INSERT_TABLE_NAME (select ID, ENTITY_ID,PROFILED_ENTITY_ID,GENERATED_DATE,SERIAL_NUMBER,ELEMENTS,1,1,0 from $SELECT_TABLE_NAME where PROFILED_ENTITY_ID='$PROFILED_ENTITY_ID');
        commit;
		SPOOL OFF;
        quit

EOF
}

main()
{
        insertSuspectFP $*
}

main $*
