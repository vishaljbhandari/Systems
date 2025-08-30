DB_USER=$1
DB_PASSWORD=$2

sqlplus -s $DB_USER/$DB_PASSWORD << EOF
@migration.sql
@../archiver_package.sql
@../cleanup_package.sql
@../alarm_closure_actions.sql

EOF
