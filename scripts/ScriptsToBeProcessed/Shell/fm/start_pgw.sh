#!/bin/bash
. /ranger/RangerRoot/sbin/rangerenv.sh
###cd /ranger/ranger/RangerRoot/bin
cd /ranger/RangerRoot/bin

export TZ=GMT+0

nohup ruby trans_service_mod_ver.rb &
