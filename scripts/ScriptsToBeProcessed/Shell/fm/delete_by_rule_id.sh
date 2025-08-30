#!/bin/bash

if [ $# -lt 1 ] ; then
	echo "Usage: $0 <RuleID> " ;
	exit 1 ;
fi

RuleID=$1

echo "RuleID --- $RuleID " >> delete_rule.log


sqlplus -l -s $DB_USER/$DB_PASSWORD << EOF >> delete_rule.log
SET SERVEROUTPUT ON ;
WHENEVER OSERROR EXIT 5 ;

drop table id_backup ;
delete from thresholds where rule_key = (select key from rules where id = $RuleID) ; 

create table id_backup as select expression_id from filter_tokens where filter_id in (select filter_id from filters_rules where rule_id = $RuleID) and expression_id is not null ;
delete from filter_tokens where filter_id in (select filter_id from filters_rules where rule_id = $RuleID) ;
delete from expressions where id in (select expression_id from id_backup) ;
drop table id_backup ;

create table id_backup as select filter_id from filters_rules where rule_id = $RuleID ;
delete from filters_rules where rule_id = $RuleID ;
delete from filters where id in (select filter_id from id_backup) ;
drop table id_backup ;

delete from rules_subscriber_groups where rule_id = $RuleID ;
delete from hotlist_rules where rule_id = $RuleID ;
delete from rules_tags where rule_id = $RuleID ;
delete from rule_notification_maps where rule_id = $RuleID ;
delete from fraud_types_rules where rule_id = $RuleID ;
delete from record_configs_rules where rule_id = $RuleID ;
delete from rule_priority_maps where rule_id = $RuleID ;
delete from actions_rules where rule_id = $RuleID ;
delete from analyst_actions_rules where rule_id = $RuleID ;
delete from networks_rules where rule_id = $RuleID ;

delete from match_configurations_networks where match_configuration_id in (select id from match_configurations where rule_id = $RuleID) ;
delete from match_field_configurations where match_config_id in (select id from match_configurations where rule_id = $RuleID) ;
delete from match_configurations where rule_id = $RuleID ;

delete from rules where id = $RuleID ;

commit ;
quit ;
EOF

