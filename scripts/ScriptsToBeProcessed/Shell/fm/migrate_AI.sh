#! /bin/bash

. rangerenv.sh

echo "Running AI Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_AI.sql > migrate_AI.log
