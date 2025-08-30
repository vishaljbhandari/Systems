#!/bin/bash
i=1
while
dd if=/dev/zero of=file$i bs=10K count=1 2>/dev/null 1>&2
do
i=$((i+1))
done
echo created $i files 

