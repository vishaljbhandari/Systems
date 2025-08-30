#!/bin/bash

. $RANGERHOME/sbin/rangerenv.sh

FraudNumber=$1
scriptlauncher $RANGERHOME/bin/populateasncdr.sh $FraudNumber 2&>1 > /dev/null

$RANGERHOME/sbin/recorddispatcher -r "DB://ASN_WORLD_NUMBERS" -c "ASN" 
logger -t 'ASNPopulation' "Updated ASN for Fraud Number '$FraudNumber'"
