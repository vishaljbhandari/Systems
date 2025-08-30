#!/bin/bash

if [ $# -ne 1 ]
then
	exit 0
fi

sleep $1
exit $1 
