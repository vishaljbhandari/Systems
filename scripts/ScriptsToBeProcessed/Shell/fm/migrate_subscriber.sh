#! /bin/bash

. rangerenv.sh

echo "Running Subscriber Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_subscriber.sql > migrate_subscriber.log
