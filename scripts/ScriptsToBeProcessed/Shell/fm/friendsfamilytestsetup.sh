

sqlplus -s $DB_USER/$DB_PASSWORD <<EOF

	delete friends_family ;
	insert into friends_family values ('+919234567888', '+9123425334664,+91234235364656,+91342353464366');
	insert into friends_family values ('+919234567889', '+9123425334674,+91234235364676,+91342353464376');
	insert into friends_family values ('+919234567887', '+9123425334684,+91234235364686,+91342353464386');
	insert into friends_family values ('+919234567886', '+9123425334694,+91234235364696,+91342353464396');
	commit ;
EOF
