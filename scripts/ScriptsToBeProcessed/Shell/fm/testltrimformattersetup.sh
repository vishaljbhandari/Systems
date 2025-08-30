sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from ltrim_formatter where formatter_name like  '%LTRIM_XXX%'  ;
insert into ltrim_formatter values ('LTRIM_XXX','XXX') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
