#! /bin/bash

sqlplus -l $DB_USER/$DB_PASSWORD < migrations-after-14032007.sql 
sqlplus -l $DB_USER/$DB_PASSWORD < alarmgenerator_procedures.sql
sqlplus -l $DB_USER/$DB_PASSWORD < cleanup_package.sql
sqlplus -l $DB_USER/$DB_PASSWORD < archiver_package.sql
sqlplus -l $DB_USER/$DB_PASSWORD < highriskdestination_summary_procedure.sql
sqlplus -l $DB_USER/$DB_PASSWORD < lifestyleprofileandalarmgenerator_procedures.sql
sqlplus -l $DB_USER/$DB_PASSWORD < utility_package.sql
