#! /bin/bash

if [ $# -lt 1 ] ; then
    exit 2
fi

AQ_ID=$1
PRESENT_DIR=test/mocks/test

rm -f $PRESENT_DIR/*.hmm

touch $PRESENT_DIR/$AQ_ID.hmm
