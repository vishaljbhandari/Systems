#!/bin/env bash

db2load()
{
datafile=$2
controlfile=$1
badfile=$3
COMMAND=`cat $controlfile`
COMMAND=`echo $COMMAND |sed -e "s#:DATA#$datafile#g; s#:BAD#$badfile#g"`
echo $COMMAND
}
