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
createsynonym DUDUBAI_UNF_CONVERSION
createsynonym ACCOUNT
createsynonym DUDUBAI_SS7_OPC_DPC
createsynonym DU_DUBAI_LOCAL_CALL_CHECK
createsynonym CONFIGURATIONS


fi
exit 0

