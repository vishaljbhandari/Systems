sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from datasource_info where datasource_name like  '%GENERICDS%'  ;
delete from ds_filter where datasource_name like  'GENERICDS%'  ;
delete from prefix_filter where filter_name like '%PREFIX_200%' ;
delete from match_field_filter where Filter_name like '%MATCH_CALLED_NUMBER%' ;
delete from partial_cdr_configuration where filter_name like '%PARTIAL_CDR_STITCH%' ;

insert into datasource_info values('GENERICDS',1) ;
insert into ds_filter values ('GENERICDS',1,1,'MATCH_CALLED_NUMBER','N') ;
insert into ds_filter values ('GENERICDS',2,2,'PREFIX_200_TEST','N') ;
insert into ds_filter values ('GENERICDS',3,3,'PARTIAL_CDR_STITCH','Y') ;
insert into prefix_filter values ('PREFIX_200_TEST','CALLER_NUMBER', '200','Y') ;
insert into MATCH_FIELD_FILTER values ('MATCH_CALLED_NUMBER', 'CALLER_NUMBER', 'CALLED_NUMBER','Y') ;
insert into PARTIAL_CDR_CONFIGURATION values ('PARTIAL_CDR_STITCH', 24, 'ASN_CALL_IDENTIFICATION_NUMBER', 'ASN_DATE_FOR_START_OF_CALL', 'ASN_TIME_FOR_START_OF_CALL', 'ASN_PARTIAL_OUPUT_REC_NUM', 'ASN_LAST_PARTIAL_OUTPUT','ASN_DURATION','ASN_TIME_STAMP','ASN_IS_ATTEMPTED', '%Y/%m/%d %H:%M:%S') ;

delete from datasource_info  where datasource_name like  'IGENERICDS%'  ;
insert into datasource_info values('IGENERICDS',1) ;
insert into ds_filter values ('IGENERICDS',1,77,'PREFIX_2000','N') ;

delete from datasource_info  where datasource_name like  'XGENERICDS%'  ;
insert into datasource_info values('XGENERICDS',1) ;
insert into ds_filter values ('XGENERICDS',1,2,'PREFIX_2000','N') ;

commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
