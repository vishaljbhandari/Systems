sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from validator where datasource_name like  'GENERICDS%'  ;
insert into validator values ('GENERICDS','X',1,1, '') ;
insert into validator values ('GENERICDS','X',2,2, '') ;
insert into validator values ('GENERICDS','X',3,3, '') ;
insert into validator values ('GENERICDS','Y',1,1, '') ;
insert into validator values ('GENERICDS','Y',2,3, '') ;
insert into validator values ('GENERICDS','Z',1,1, '') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
