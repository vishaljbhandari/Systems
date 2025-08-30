

sqlplus -s $DB_USER/$DB_PASSWORD <<EOF

	delete networks_sdr_rates ;
	delete sdr_rates ;

	insert into sdr_rates values (1024, to_date('01/09/2010 00:00:00', 'dd/mm/yyyy hh24:mi:ss'), to_date('10/09/2010 23:59:59', 'dd/mm/yyyy hh24:mi:ss'), 10);
	insert into sdr_rates values (1025, to_date('12/09/2010 00:00:00', 'dd/mm/yyyy hh24:mi:ss'), to_date('20/09/2010 23:59:59', 'dd/mm/yyyy hh24:mi:ss'), 20);

	insert into networks_sdr_rates select id, 1024 from networks ;
	insert into networks_sdr_rates select id, 1025 from networks ;
	
	commit ;
EOF
