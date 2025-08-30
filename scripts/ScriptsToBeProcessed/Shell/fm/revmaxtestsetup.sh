#! /bin/bash

mkdir -p $HOME/revmaxroot/lib/
mkdir -p $HOME/revmaxroot/log/
mkdir -p $HOME/revmaxroot/sbin

touch $HOME/revmaxroot/sbin/revmax.conf

. $REVMAXHOME/bin/revmaxenv.sh

echo DB_USER=$DB_USER > $HOME/revmaxroot/sbin/revmax.conf
echo DB_PASSWORD=$DB_PASSWORD >> $HOME/revmaxroot/sbin/revmax.conf

# for marathon file lister
echo HOME=$HOME >> $HOME/revmaxroot/sbin/revmax.conf

