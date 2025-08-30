#!/bin/bash

if [ "$1" != "" ]  #if parameter exists, use as base folder
   then cd "$1"
fi
pwd
find . -mindepth 1 -type d -empty
cd -
