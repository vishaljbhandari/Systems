$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5;
whenever oserror exit 5;
set heading off
spool testsubscribercleanupV.dat

select 1 from dual where 0 = (select count(*) from account where id=1810) ;
select 1 from dual where 0 = (select count(*) from subscriber where phone_number='+919820336300') ;

select 1 from dual where 1 = (select count(*) from account where id=1811) ;
select 1 from dual where 0 = (select count(*) from subscriber where phone_number='+919820336301') ;
select 1 from dual where 1 = (select count(*) from subscriber where phone_number='+919820336304') ;

select 1 from dual where 1 = (select count(*) from account where id=1812) ;
select 1 from dual where 1 = (select count(*) from subscriber where phone_number='+919820336302') ;

select 1 from dual where 1 = (select count(*) from account where id=1813) ;
select 1 from dual where 1 = (select count(*) from subscriber where phone_number='+919820336303') ;

select 1 from dual where 0 = (select count(*) from fp_entityid_16_v where profiled_entity_id = 1810 and entity_id = 16 and version = 0) ; 

select 1 from dual where 0 = (select count(*) from subscriber_neural_profile where phone_number in ('+919820336300', '+919820336301')) ;
select 1 from dual where 0 = (select count(*) from blacklist_neural_profile where phone_number in ('+919820336300', '+919820336301')) ;
select 1 from dual where 2 = (select count(*) from blacklist_neural_profile where phone_number in ('+919820336303', '+919820336304')) ;
select 1 from dual where 0 = (select count(*) from gprs_life_style_profiles where phone_number in ('+919820336300', '+919820336301')) ;
select 1 from dual where 0 = (select count(*) from usage_profiles where phone_number in ('+919820336300', '+919820336301')) ;

spool off
EOF
 
if [ $? -eq '5' ] || grep "no rows" testsubscribercleanupV.dat
then
	exitval=1
else
	exitval=0
fi
rm -f testsubscribercleanupV.dat
exit $exitval
