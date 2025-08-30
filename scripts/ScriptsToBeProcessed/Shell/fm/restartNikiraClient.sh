#!/bin/bash

. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

cd $RANGERV6_CLIENT_HOME
./nikiraclientctl.sh restart < /dev/null >& /dev/null
