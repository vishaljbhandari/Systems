#!/bin/bash
. $RUBY_UNIT_HOME/Scripts/configuration.sh
cd $SPARK_DEPLOY_DIR/bin 
./debugclient.sh "$@"
