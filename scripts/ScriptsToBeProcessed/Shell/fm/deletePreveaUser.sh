. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

sqlplus -s $PREVEA_DB_SETUP_USER/$PREVEA_DB_SETUP_PASSWORD << EOF

set termout off
set heading off
set echo off
delete from users where id >= 14000;
truncate table sessions;
commit;
quit

EOF

