#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh

mv $WATIR_SERVER_HOME/clientFiles/* $RANGERV6_CLIENT_HOME/../$2
