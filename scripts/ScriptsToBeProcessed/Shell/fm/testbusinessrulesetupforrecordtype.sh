sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 4;
whenever oserror exit 4;

delete from BUSINESSRULE where datasource_name like  '%"RECORDTYPE_TEST"%'  ;
insert into BUSINESSRULE values ('"RECORDTYPE_TEST"',1,2,'"RECORDTYPE_TEST"', 'brcms40') ;
commit;
EOF

if test $? -eq 4
then
	exit 1
fi

exit 0
