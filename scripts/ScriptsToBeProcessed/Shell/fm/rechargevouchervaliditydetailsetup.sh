$ORACLE_HOME/bin/sqlplus $DB_USER/$DB_PASSWORD << EOF

whenever oserror exit 1 ;
whenever sqlerror exit 2 ;

delete from voucher_validity_detail ;

quit success commit ;
EOF
