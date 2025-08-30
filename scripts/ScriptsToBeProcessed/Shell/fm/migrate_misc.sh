#! /bin/bash

. rangerenv.sh

echo "Running Misc Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_misc.sql > migrate_misc.log
