#!/bin/bash

. $RUBY_UNIT_HOME/Scripts/configuration.sh
. $RANGERHOME/sbin/rangerenv.sh

echo "Updating the table entries for AT Setup..."
sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF

        set feedback off
        	delete from FILTER_INDEX_FIELDS;
		update configurations set value = '0'  where config_key like 'ALARM.POPUP.WINDOW.ENABLED';
		update configurations set value = '1'  where config_key='ALARMDENORMALIZER.TRIGGER_AFTER_SLEEP';
		update configurations set value = '1' where config_key='CDRIDIncrement';
		update DS_GRANULARITY_MAPS set THRESHOLD_IN_SECONDS=8640000;
		update field_configs set ds_category=ds_category||'ENTITY_DATA' where record_config_id=3 and field_id=33;
		update field_configs set ds_category=ds_category||'ENTITY_DATA' where record_config_id=4 and field_id in (29,12);
		update field_configs set ds_category=ds_category||'ENTITY_DATA' where record_config_id=5 and field_id in (3,4,5,6,7,8,11,12,13,14,15,17);
		update field_configs set ds_category=ds_category||'ENTITY_DATA' where record_config_id=6 and field_id in (3,5);
		update rules set IS_ENABLED=1 where  NAME='RuleForUpdateIMEI';
		update rules set IS_ENABLED=1 where  NAME='RuleForPrepaidUpdation';
		update QUERY_FRAMEWORK_CONFIGS set VALUE ='9999' where CONFIG_KEY='QUERY_COST_THRESHOLD';
		update IN_MEMORY_COUNTER_DETAILS set DELTA_READ_BUFFER_SIZE='120';
		update inmemory_counter_configs set value='1' where config_key='CDRDETAIL_UPDATE_INTERVAL';
		update INMEMORY_COUNTER_CONFIGS set VALUE='150' where config_key='FUTURE_RECORD_BUFFER_SIZE';
	 	update IN_MEMORY_COUNTER_DETAILS set BATCH_COUNT='0';
		update record_configs set IS_DISTRIBUTED_CACHE=0 where id in(3,4); 
		drop trigger $DB_SETUP_USER.COUNTERDISTRIBUTIONTRIGGER;
		commit;
		quit

EOF

if [ $IS_MLH_ENABLED -eq 1 ]
then
sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF

        set feedback off
		update INMEMORY_COUNTER_CONFIGS set VALUE='250' where config_key='FUTURE_RECORD_BUFFER_SIZE';
        commit;
        quit

EOF
fi

if [ $IS_RATING_ENABLED -eq 1 ]
then
sqlplus -s  $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF
	set feedback off
	@test_rater_configurations.sql
	quit
EOF
fi
echo "Completed updating the table entries for AT Setup..."

bash $SERVER_CHECKOUT_PATH/Processor/InMemoryCounterManager/compile.sh $SERVER_CHECKOUT_PATH/Processor/InMemoryCounterManager/clearsegments.cc
mv $RUBY_UNIT_HOME/Scripts/a.out $RUBY_UNIT_HOME/Scripts/clearsegments
if [ $IS_CDM_ENABLED -eq 1 ]
then
cp tc1.sh tc2.sh tc3.sh tc4.sh classpath.sh invoke_debugclient.sh $SPARK_DEPLOY_DIR/bin
else
sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD << EOF
		update record_configs set IS_DISTRIBUTED_CACHE=0 where id in(3,4); 
EOF
fi
cp programmanager.conf.tcp.autodisabled $RANGERHOME/sbin
cp programmanager.conf.tcp.autoenabled $RANGERHOME/sbin
