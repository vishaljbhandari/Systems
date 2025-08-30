#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

kill -HUP `ps -fu$LOGNAME | grep httpd | grep -v grep | tr -s " " | cut -d " " -f3 | grep -v "^1$" | uniq`
sleep 2