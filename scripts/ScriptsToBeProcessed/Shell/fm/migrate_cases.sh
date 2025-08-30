#! /bin/bash

. rangerenv.sh

echo "Running Cases Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_cases.sql > migrate_cases.log
