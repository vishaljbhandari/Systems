#!/bin/bash
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

if [ -f $* ]
then
    cd `dirname $*`
    bash `basename $*`
else
    echo "Set the correct path for DataGenerator.."
fi
