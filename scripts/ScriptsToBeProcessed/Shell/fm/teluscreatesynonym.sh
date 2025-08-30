#!/bin/bash

rangerdbname=$1
username=$2
password=$3
function createsynonym()
{
tablename=$1
sqlplus -s $username/$password << EOF
CREATE OR REPLACE SYNONYM $1 FOR $rangerdbname.$tablename;
EOF
}

if [ $# -eq 0 ]
then
echo "Error: No arguments provided."
exit 1
else
createsynonym SUBSCRIBER
createsynonym CDR_SERVICE
createsynonym UNF_CONVERSION
createsynonym ACCOUNT
createsynonym TELUS_POINTCODES
createsynonym TELUS_LOCAL_CALL_LOOKUP
createsynonym TELUS_SPECIAL_NUMBER
createsynonym TELUS_OCN_NPA
createsynonym TELUS_NPA
fi
exit 0
