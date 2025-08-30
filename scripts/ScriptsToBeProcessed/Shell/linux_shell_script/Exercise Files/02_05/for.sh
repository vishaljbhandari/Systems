#!/usr/bin/env bash

NAMES=$@

for NAME in $NAMES
do
  echo "Hello $NAME"
done

echo "for loop terminated"
exit 0
