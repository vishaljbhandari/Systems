
$ORACLE_HOME/bin/sqlplus $DB_USER/$DB_PASSWORD << EOF

whenever oserror exit 1 ;
whenever sqlerror exit 1 ;

delete from ds_ascii_input_configuration ;

quit ;
EOF
