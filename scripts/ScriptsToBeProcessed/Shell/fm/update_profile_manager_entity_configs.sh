#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh
dir_name=$1
sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF
set feedback off;
update PM_INSTANCE_CONFIGS set last_run_ref_time = SYSDATE-1;
commit;
exit;
EOF
