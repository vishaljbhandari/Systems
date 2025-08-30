#! /bin/bash

if [ $# -ne 3 ]; then
    echo "Usage: require 3 parameters: RangerDBUsername,SparkDB_Username,SparkDB_Password"
    exit 5
fi

. $RANGERHOME/sbin/rangerenv.sh

echo "Running Telus/Scripts/telus-diamond-lookup-db.sql"
sqlplus -s $2/$3 < telus-diamond-lookup-db.sql
if [ $? -ne 0 ]; then
    exit 5
fi

echo "Running Telus/Scripts/teluscreatesynonym.sh"
bash teluscreatesynonym.sh $1 $2 $3 
if [ $? -ne 0 ]; then
    exit 5
fi
