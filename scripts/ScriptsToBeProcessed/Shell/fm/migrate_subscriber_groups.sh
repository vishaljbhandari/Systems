#! /bin/bash

. rangerenv.sh

echo "Running Subscriber Groups Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_subscriber_groups.sql > migrate_subscriber_groups.log
