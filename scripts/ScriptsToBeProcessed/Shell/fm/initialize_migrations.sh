#! /bin/bash

. rangerenv.sh

echo "Initializing..."
sqlplus -l -s $DB_USER/$DB_PASSWORD < migration_db_link.sql > migration_db_link.log
