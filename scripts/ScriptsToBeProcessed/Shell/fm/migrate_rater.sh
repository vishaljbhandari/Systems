#! /bin/bash

. rangerenv.sh

echo "Running Rater DB Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_rater.sql > migrate_rater.log
