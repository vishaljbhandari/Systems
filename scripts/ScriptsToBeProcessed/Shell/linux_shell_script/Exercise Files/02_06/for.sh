#!/usr/bin/env bash

NAMES=$@

for NAME in $NAMES
do
  if [ $NAME = "Tracy" ]
  then
    continue
  fi
  echo "Hello $NAME"
done

echo "for loop terminated"
exit 0
