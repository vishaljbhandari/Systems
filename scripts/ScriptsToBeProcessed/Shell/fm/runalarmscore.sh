#! /bin/bash

cat $1 | grep -v "^#" | ./alarmscore -s
