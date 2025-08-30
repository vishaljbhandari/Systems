sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from datasource_info where datasource_name = 'MTX12LNP' ; 
insert into datasource_info values ('MTX12LNP',1) ;
commit;
EOF

if test $? -eq 5
then
    exit 1
fi

exit 0

