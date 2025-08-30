sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from unf_formatter where formatter_name like  'UNF%'  ;
insert into unf_formatter values ('UNF','1','+1') ;
insert into unf_formatter values ('UNF','2','+2') ;
insert into unf_formatter values ('UNF','3','+3') ;
insert into unf_formatter values ('UNF','4','+4') ;
insert into unf_formatter values ('UNF','5','+5') ;
insert into unf_formatter values ('UNF','6','+6') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
