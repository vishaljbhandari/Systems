DB_USER=$1
DB_PASSWORD=$2

for i in `cat spark_support_tables.txt`
do
	sqlplus -s $DB_USER/$DB_PASSWORD << EOF

	set serveroutput on ;
	DECLARE
	RCONFIG_ID NUMBER(20) ;
	MAX_POS_ID NUMBER(20) ;
	FILE_WRITE_RULE_ID NUMBER(20) ;
	TABLE_NAME VARCHAR2(64) ;
	BEGIN

		BEGIN
		SELECT ID INTO RCONFIG_ID FROM RECORD_CONFIGS WHERE TNAME = '$i' ;
		EXCEPTION
		   WHEN NO_DATA_FOUND THEN
		   DBMS_OUTPUT.PUT_LINE('Record Configs Entry Not Found for Table Name :  "$i"') ;
		END ;

		BEGIN
		SELECT MAX(POSITION) INTO MAX_POS_ID FROM FIELD_CONFIGS WHERE RECORD_CONFIG_ID = RCONFIG_ID ;
		EXCEPTION
		   WHEN NO_DATA_FOUND THEN
		   DBMS_OUTPUT.PUT_LINE('Field Configs Entry Not Found For Record Config ID :' || RCONFIG_ID) ;
		END ;

		BEGIN
		SELECT ID INTO FILE_WRITE_RULE_ID FROM RULES WHERE NAME = 'File Write Action' ;
		EXCEPTION
		   WHEN NO_DATA_FOUND THEN
		   DBMS_OUTPUT.PUT_LINE('File Write Action Rule Not Found') ;
		END ;

		TABLE_NAME := lower('$i') ;

		UPDATE CONFIGURATIONS SET CONFIG_KEY ='CLEANUP.RECORDS.nik_'||TABLE_NAME||'.OPTIONS' where CONFIG_KEY = 'CLEANUP.RECORDS.$i.OPTIONS';

		UPDATE RECORD_CONFIGS SET IS_SPARK_SUPPORT=1,TNAME=lower('NIK_$i'),IS_PARTITIONED=1 WHERE ID = RCONFIG_ID ;
		UPDATE FIELD_CONFIGS SET DERIVATIVE_FUNCTION=NULL ,POSITION= (MAX_POS_ID + 1) where RECORD_CONFIG_ID = RCONFIG_ID and upper(DATABASE_FIELD) ='DAY_OF_YEAR' ;
		UPDATE FIELD_CONFIGS SET DERIVATIVE_FUNCTION=NULL ,POSITION= (MAX_POS_ID + 2) where RECORD_CONFIG_ID = RCONFIG_ID and upper(DATABASE_FIELD) ='HOUR_OF_DAY' ;
		UPDATE FIELD_CONFIGS SET DERIVATIVE_FUNCTION=NULL ,POSITION= (MAX_POS_ID + 3),DATABASE_FIELD = 'XDR_ID' where RECORD_CONFIG_ID = RCONFIG_ID and upper(DATABASE_FIELD) ='ID' ;
		UPDATE FIELD_CONFIGS SET DATABASE_FIELD='VAL' where RECORD_CONFIG_ID = RCONFIG_ID and upper(DATABASE_FIELD) ='VALUE' ;
		UPDATE RECORD_VIEW_CONFIGS SET ORDER_BY= 'XDR_ID DESC' where RECORD_CONFIG_ID = RCONFIG_ID ;
		DELETE FROM RECORD_CONFIGS_RULES WHERE RULE_ID = FILE_WRITE_RULE_ID and RECORD_CONFIG_ID = RCONFIG_ID ;

	EXCEPTION
	   WHEN OTHERS THEN
	   DBMS_OUTPUT.PUT_LINE('Unknown DB Exception') ;
	   RAISE ;
	   ROLLBACK ;
	END ;
/

	DROP TABLE $i ;

commit ;
EOF
done

	sqlplus -s $DB_USER/$DB_PASSWORD << EOF

		insert into CURR_PART_INFOS(select CURR_PART_INFO_SEQ.nextval,id,'0:0:0' from record_configs where IS_SPARK_SUPPORT=1) ;
		insert into record_configs_rules(rule_id, record_config_id) (select rule.id,rc.id from record_configs rc , rules rule where is_spark_support=1 and rule.name ='Update CurrentPartitionInfo') ;

------- Removing Subscriber Store Rule as SubscriberLess and Invalid Subscriber Insertion in Subscriber Table is Handled by Spark Parsing Stage

		delete from filter_tokens where filter_id in (select filter_id from filters_rules where rule_id = ( select id from rules where name = 'SubscriberStore')) ;
        delete from actions_rules where rule_id = (select id from rules where name = 'SubscriberStore') ;
        delete from filters_rules where rule_id = (select id from rules where name = 'SubscriberStore') ;

        delete from record_configs_rules where rule_id =(select id from rules where name = 'SubscriberStore') ;
        delete from rule_priority_maps where rule_id =(select id from rules where name = 'SubscriberStore') ;
        delete from rules_tags where rule_id =(select id from rules where name = 'SubscriberStore') ;
		delete from rule_filter_descriptions where rule_id =(select id from rules where name = 'SubscriberStore') ;
        delete from rules where name = 'SubscriberStore' ;

commit ;
EOF

echo "delete from SPARK_SYNONYM_MAPS ;" > .temp_spark_map.sql

cat createsynonym.sh | grep -i '^\s*createsynonym' | awk -F' ' "{
tab=\$2
if (\$3 ==\"\")
    syn=\"nik_\"\$2;
else
    syn=\$3;
printf(\"insert into SPARK_SYNONYM_MAPS (TABLE_NAME, SYNONYM_NAME) values (upper('%s'),lower('%s')) ;\\n\", tab,syn) ;
}" >> .temp_spark_map.sql
echo "commit ;" >> .temp_spark_map.sql

sqlplus -s $DB_USER/$DB_PASSWORD < .temp_spark_map.sql
rm -f .temp_spark_map.sql
