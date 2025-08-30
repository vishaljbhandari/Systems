#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh
cd $DBSETUP_HOME

if [ $DB_TYPE == "1" ]
then
sqlplus $DB_SETUP_USER/$DB_SETUP_PASSWORD < lifestyleprofileandalarmgenerator_procedures.sql
else
clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME < lifestyleprofileandalarmgenerator_procedures.sql
fi
