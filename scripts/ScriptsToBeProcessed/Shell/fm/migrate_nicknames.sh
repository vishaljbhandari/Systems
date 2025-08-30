#! /bin/bash

. rangerenv.sh

echo "Running Nickname Migration..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migrate_nicknames.sql > migrate_nicknames.log
