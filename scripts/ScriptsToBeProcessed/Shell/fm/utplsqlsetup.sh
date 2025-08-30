#!/bin/bash

if [ $# -lt 1 ]
then
    echo "Usage  $0 user_name [password]"
    exit
fi

User=$1
Password=$2


if [ "$Password" = "" ]; then
    Password=$User ;
fi
TEST_DIR=`dirname $0`
cd $TEST_DIR/utPLSQL/Code
sqlplus -s -l $User/$Password <<EOF
set serveroutput on 100000 ;

@ut_i_do uninstall
@ut_i_do install
exec utconfig.setPrefix ('test_') ;
exec utConfig.setdir('$TEST_DIR') ;
EOF
