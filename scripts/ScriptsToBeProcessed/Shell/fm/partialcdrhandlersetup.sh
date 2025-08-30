

sqlplus -s $DB_USER/$DB_PASSWORD <<EOF

	delete partial_cdr_info ;	
	commit ;
EOF
