#!/bin/bash

if [ $# -lt 1 ] ; then
	echo "Usage: $0 <User Name> " ;
	exit 1 ;
fi

UserName=$1

sqlplus -l -s $DB_USER/$DB_PASSWORD << EOF >> temp_spool.log
    WHENEVER OSERROR  EXIT 6 ;
    WHENEVER SQLERROR EXIT 5 ;
    SPOOL ruleid.lst
        SELECT trim(value) FROM user_options where option_id = 'ManualPrecheckRuleID' and user_id = '$UserName' ; 
        SPOOL OFF ;
EOF

cat temp_spool.log | grep "no rows selected" > /dev/null 2>&1
norows=`echo $?`

if [ $norows -ne 0 ]
then
	tRuleID=`grep "[0-9][0-9]*" ruleid.lst`
	RuleID=`echo $tRuleID | tr -d " "`
	./delete_by_rule_id.sh $RuleID
fi

sqlplus -l -s $DB_USER/$DB_PASSWORD << EOF >> temp_spool1.log
    WHENEVER OSERROR  EXIT 6 ;
    WHENEVER SQLERROR EXIT 5 ;
    SPOOL ruleid1.lst
        SELECT trim(value) FROM user_options where option_id = 'AlarmLinkPrecheckRuleID' and user_id = '$UserName' ; 
        SPOOL OFF ;
EOF

cat temp_spool1.log | grep "no rows selected" > /dev/null 2>&1
norows=`echo $?`

if [ $norows -ne 0 ]
then
	tRuleID=`grep "[0-9][0-9]*" ruleid1.lst`
	RuleID=`echo $tRuleID | tr -d " "`
	./delete_by_rule_id.sh $RuleID
fi

rm temp_spool.log temp_spool1.log ruleid.lst ruleid1.lst



sqlplus -l -s $DB_USER/$DB_PASSWORD << EOF >> delete_users.log
SET SERVEROUTPUT ON ;
WHENEVER OSERROR EXIT 5 ;

delete from RANGER_GROUPS_RANGER_USERS where RANGER_USER_ID in (select id from ranger_users where name = '$UserName') ;
delete from PASSWORDS where RANGER_USER_NAME  = '$UserName' ;
delete from NETWORKS_RANGER_USERS where RANGER_USER_ID in (select id from ranger_users where name = '$UserName') ;
delete from user_options where user_id  = '$UserName' ;
delete from ranger_users where name = '$UserName' ;

delete from filters_rules where rule_id in (select id from rules where category = 'COLORING_RULES' and user_id = '$UserName' ) ;
delete from record_configs_rules where rule_id in (select id from rules where category = 'COLORING_RULES' and user_id = '$UserName' ) ;
delete from rule_color_maps where rule_id in (select id from rules where category = 'COLORING_RULES' and user_id = '$UserName' ) ;
delete from rules where category = 'COLORING_RULES' and user_id = '$UserName' ;


commit ;
quit ;
EOF

