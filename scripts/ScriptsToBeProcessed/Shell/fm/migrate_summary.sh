#! /bin/bash

. rangerenv.sh

echo "Running SUMMARY Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_summary.sql > migrate_summary.log
