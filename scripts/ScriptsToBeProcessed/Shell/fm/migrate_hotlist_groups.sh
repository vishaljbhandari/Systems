#! /bin/bash

. rangerenv.sh

echo "Running Hotlist Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_hotlist_groups.sql > migrate_hotlist_groups.log
