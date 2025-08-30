sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;
delete from datasource_info where datasource_name like  '%GENERICDS%'  ;
insert into datasource_info values('GENERICDS',1) ;

insert into formatter values ('GENERICDS','X',1,1,'TRIM') ;
insert into formatter values ('GENERICDS','X',2,2,'UNF') ;
insert into formatter values ('GENERICDS','X',3,3,'MAP') ;
insert into formatter values ('GENERICDS','X',4,4,'DATE') ;
insert into formatter values ('GENERICDS','Y',1,1,'FILLERTRIM') ;
insert into formatter values ('GENERICDS','Y',2,3,'MAP') ;
insert into formatter values ('GENERICDS','Z',1,1,'TRIM') ;
delete from trim_formatter where formatter_name like  'TRIM%'  ;
delete from trim_formatter where formatter_name like  'FILLERTRIM%'  ;
insert into trim_formatter values ('FILLERTRIM','F') ;
insert into trim_formatter values ('TRIM',null) ;
delete from date_formatter where formatter_name like  'DATE%'  ;
insert into date_formatter values ('DATE','%d%m%Y%H%M%S','%d/%m/%Y %H:%M:%S') ;
delete from map_formatter where formatter_name like  'MAP%'  ;
insert into map_formatter values ('MAP','F','G') ;
insert into map_formatter values ('MAP','H','I') ;
delete from unf_formatter where formatter_name like  'UNF%'  ;
insert into unf_formatter values ('UNF','1','+1') ;
insert into unf_formatter values ('UNF','2','+2') ;
insert into unf_formatter values ('UNF','3','+3') ;
insert into unf_formatter values ('UNF','4','+4') ;
insert into unf_formatter values ('UNF','5','+5') ;
insert into unf_formatter values ('UNF','6','+6') ;

delete from datasource_info  where datasource_name like  'IGENERICDS%'  ;
insert into datasource_info values('IGENERICDS',1) ;
insert into formatter values ('IGENERICDS','X',1,77,'TRIM') ;

delete from datasource_info  where datasource_name like  'XGENERICDS%'  ;
insert into datasource_info values('XGENERICDS',1) ;
insert into formatter values ('XGENERICDS','X',1,1,'FTRIM') ;
commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
