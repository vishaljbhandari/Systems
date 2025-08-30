#!/bin/bash

User=$1
Password=$2


if [ $# -lt 1 ]
then
    echo "Usage  $0 user_name [password]"
    exit
fi


if [ "$Password" = "" ]; then
    Password=$User ;
fi




echo
echo "------------------------------------------------"
echo "Running Migration from Nikira 7.3A to Nikira 7.3"
echo "------------------------------------------------"
echo

sh remove_stat_rule_cron.sh

LogFile=migration_after_7.3A.log
> $LogFile

Files="
remove_schedule_entries.sql
"

for file in $Files
do
	sqlplus -l -s $User/$Password < $file | tee -a $LogFile
done

echo "---------------Migration completed---------------"
echo "Check $LogFile for details"
