sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from duplicate_cdr_file_config ;
delete from cdr_file_detail ;

insert into duplicate_cdr_file_config (ds_type, time_band) values ('Test', 5) ;
insert into duplicate_cdr_file_config (ds_type, time_band) values ('Test1', 5) ;

insert into cdr_file_detail (ds_type, file_name, time_stamp, file_size) values ('Test', 'newfile', 
		to_date('10/10/2004 06:59:59', 'dd/mm/yyyy hh24:mi:ss'), 34567) ;
insert into cdr_file_detail (ds_type, file_name, time_stamp, file_size) values ('Test', 'newfile', 
		to_date('10/10/2004 07:59:59', 'dd/mm/yyyy hh24:mi:ss'), 23456) ;
insert into cdr_file_detail (ds_type, file_name, time_stamp, file_size) values ('Test', 'newfile1', 
		to_date('10/10/2004 08:59:59', 'dd/mm/yyyy hh24:mi:ss'), 23456) ;
insert into cdr_file_detail (ds_type, file_name, time_stamp, file_size) values ('Test', 'newfile2', 
		to_date('10/10/2004 09:59:59', 'dd/mm/yyyy hh24:mi:ss'), 23456) ;
insert into cdr_file_detail (ds_type, file_name, time_stamp, file_size) values ('Test1', 'newfile', 
		to_date('10/10/2004 06:59:59', 'dd/mm/yyyy hh24:mi:ss'), 34567) ;

commit;
EOF
if test $? -eq 5
then
    exitval=1
else
    exitval=0
fi

exit $exitval
