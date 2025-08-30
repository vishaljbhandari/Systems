#! /bin/bash

if [ $# -lt 1 ] ; then
    exit 2
fi

NETWORK_ID=$1
NON_FRAUD_SUBSCRIBER=0
FRAUD_SUBSCRIBER=1

if [ -z $2 ]
then
    SUB_TYPE=$FRAUD_SUBSCRIBER
else
    SUB_TYPE=$2
fi

if [ -z $3 ]
then
    SUB_ID=0
else
    SUB_ID=$3
fi

PRESENT_DIR=test/mocks/test

rm -f $PRESENT_DIR/*.lsp

touch $PRESENT_DIR/$NETWORK_ID.lsp
touch $PRESENT_DIR/$SUB_TYPE.lsp
touch $PRESENT_DIR/$SUB_ID.lsp
