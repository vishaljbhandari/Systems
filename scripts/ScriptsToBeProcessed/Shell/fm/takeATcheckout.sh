#!/bin/bash

echo "Taking AT checkout ..."
AT_CHECKOUTPATH=`awk '{FS = "="} /AT_CHECKOUTPATH\ *=/ {print $2}' atSetup.conf`
svn co $AT_CHECKOUTPATH $WATIR_HOME