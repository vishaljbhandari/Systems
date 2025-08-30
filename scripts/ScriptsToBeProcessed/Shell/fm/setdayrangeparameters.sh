
$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
set serveroutput on ;

delete from configurations where config_key = 'AI.CUMREC_VOICE_LAST_PROCESSED_DAY_OF_YEAR_1' ;
delete from configurations where config_key = 'LAST_CDR_TIME_0' ;
delete from configurations where config_key = 'LAST_CDR_TIME_1' ;

insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'AI.CUMREC_VOICE_LAST_PROCESSED_DAY_OF_YEAR_1', '$1') ;
insert into configurations(id, config_key, value) values(configurations_seq.nextval, 'LAST_CDR_TIME_0', '$2') ;
insert into configurations (id, config_key) values(configurations_seq.nextval, 'LAST_CDR_TIME_1') ;

commit ;
quit ;
EOF

if test $? -eq 5 
then
    exitval=1
else
    exitval=0
fi

exit $exitval
