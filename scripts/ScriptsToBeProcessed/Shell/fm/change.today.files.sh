#! /bin/bash

dir=$1
echo $1

find $dir -mtime 0 > chnaged.files
