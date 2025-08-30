#!/bin/bash

. $COMMON_MOUNT_POINT/Conf/rangerenv.sh
. $WATIR_SERVER_HOME/Scripts/configuration.sh

find $COMMON_MOUNT_POINT/InMemory -type f -exec rm -f {} \;
$WATIR_SERVER_HOME/Scripts/clearsegments 1 0 5 7 32 64 96 128 160 192 224
$WATIR_SERVER_HOME/Scripts/clearsegments 1 1 5 7 32 64 96 128 160 192 224
$WATIR_SERVER_HOME/Scripts/clearsegments 1000 0 5 7 32 64 96 128 160 192 224
$WATIR_SERVER_HOME/Scripts/clearsegments 1 2 5 7 32 64 96 128 160 192 224
$WATIR_SERVER_HOME/Scripts/clearsegments 2 0 5 7 32 64 96 128 160 192 224
$WATIR_SERVER_HOME/Scripts/clearsegments 2 1 5 7 32 64 96 128 160 192 224

## Added to handle the Alarm attachment icon getting shown in ATs because of Performance fix
find $RANGERV6_CLIENT_HOME/private/Attachments/Alarm/* -type d |xargs rm -rf
