sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from regex_validator ;
insert into regex_validator values ('PHONE_NUMBER_VALIDATOR', 1, '^[+]?[0-9-]+') ;
insert into regex_validator values ('ACCT_NUMBER_VALIDATOR', 1, 'A.*') ;
insert into regex_validator values ('ACCT_NUMBER_VALIDATOR', 2, '.*[0-9]+') ;

commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
