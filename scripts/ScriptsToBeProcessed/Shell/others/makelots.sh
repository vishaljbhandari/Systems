#!/bin/bash
i=1
while
touch file$i
do
i=$((i+1))
done
echo created $i files 

