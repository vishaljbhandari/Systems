#!/bin/bash

for i in `ipcs -m | grep "$LOGNAME" | awk '{print $2}'`; do ipcrm -m $i; done
echo `ipcs -m | grep "$LOGNAME" | wc -l` "entries remain"
(cd $COMMON_MOUNT_POINT/FMSData/BitMapIndexes/ && rm -rf *)
