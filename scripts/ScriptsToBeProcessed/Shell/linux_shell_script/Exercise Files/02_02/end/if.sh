#!/usr/bin/env bash

COLOR=$1
if [ $COLOR = "blue" ]
then
  echo "The color is blue"
else
  echo "The color is NOT blue"
fi

USER_GUESS=$2
COMPUTER=50

if [ $USER_GUESS -lt $COMPUTER ]
then
  echo "You're too low"
else
  echo “You’re equal or too high”
fi
