#!/bin/bash
if [ "$#" -ne 1 ]
then
echo Usage: $0 BLOCK-SIZE
exit 1
fi
i=1
while
dd if=/dev/zero of=file$i bs=$1 count=1 2>/dev/null 1>&2
do
i=$((i+1))
done
echo created $i files 

