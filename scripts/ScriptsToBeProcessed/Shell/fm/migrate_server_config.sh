#! /bin/bash

. rangerenv.sh

echo "Running Server Config Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_server_configurations.sql > migrate_server_configurations.log
