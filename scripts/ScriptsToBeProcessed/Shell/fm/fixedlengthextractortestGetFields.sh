
$ORACLE_HOME/bin/sqlplus $DB_USER/$DB_PASSWORD << EOF

whenever oserror exit 1 ;
whenever sqlerror exit 1 ;

delete from ds_ascii_input_configuration ;

insert into ds_ascii_input_configuration values (34, 14, 1, 1) ;
insert into ds_ascii_input_configuration values (50, 10, 2, 1) ;
insert into ds_ascii_input_configuration values (60, 28, 3, 1) ;
insert into ds_ascii_input_configuration values (87, 28, 4, 1) ;
insert into ds_ascii_input_configuration values (114, 20, 5, 1) ;
insert into ds_ascii_input_configuration values (133, 20, 6, 1) ;

quit ;
EOF
