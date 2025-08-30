sqlplus $DB_USER/$DB_PASSWORD<<EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from datasource_info where datasource_name like '%ALLTEL.ROAMEX' ;
delete from parser where parser_name like 'ALLTEL%' ;
insert into datasource_info values('ALLTEL.ROAMEX',2) ;
insert into parser values('ALLTEL.ROAMEX',1,'ALLTEL.ROAMEX') ;

DELETE FROM ASN1_PARSER_CONFIGURATION WHERE PARSER_NAME = 'ALLTEL.ROAMEX' AND ID LIKE '%RECORD_HEADER_LENGTH%' ;

INSERT INTO ASN1_PARSER_CONFIGURATION VALUES ('ALLTEL.ROAMEX','RECORD_HEADER_LENGTH','w') ;

commit ;

EOF

if [ $? -ne 0 ]
then
    exit 1
fi

