#! /bin/bash

. rangerenv.sh

echo "Running DU Specific Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_DU_spec.sql > migrate_DU_spec.log
