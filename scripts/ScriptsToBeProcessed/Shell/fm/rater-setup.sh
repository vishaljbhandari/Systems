echo "Running ranger-db-Rater.sql"
sqlplus -s $DB_USER/$DB_PASSWORD @ranger-db-Rater.sql

if [ $? -ne 0 ]; then
	exit 5
fi

echo "Running ranger-exec-Rater.sql"
sqlplus -s $DB_USER/$DB_PASSWORD @ranger-exec-Rater.sql

if [ $? -ne 0 ]; then
	exit 5
fi
exit 0
