#! /bin/bash

rangerdbname=$DB_USER
rangerdbpassword=$DB_PASSWORD
username=$SPARK_DB_USER
password=$SPARK_DB_PASSWORD

sqlplus -s $DB_USER/$DB_PASSWORD << EOF > $RANGERHOME/LOG/migration_spark_21_01_2013.log
--whenever sqlerror exit 5 ;
--whenever oserror exit 6 ;
set feedback on ;
set serveroutput on ;

DROP TABLE TEM_NULLTRAFFIC_EXCLUDE;
CREATE TABLE TEM_NULLTRAFFIC_EXCLUDE(
	ID		NUMBER(10),
	CALLED_NUMBER	VARCHAR2(80),
	PRIMARY KEY (ID,CALLED_NUMBER)
);

commit ;
quit ;
EOF

if [ $? -eq 0 ]
then
    echo -e "\n Nikira Migration Completed !!!"
    echo -e "Check Log : $RANGERHOME/LOG/migration_spark_21_01_2013.log\n"
else
    echo -e "\n Nikira Migration Exited with Error !!!"
    echo -e "Check Log : $RANGERHOME/LOG/migration_spark_21_01_2013.log\n"
fi ;


sqlplus -s $SPARK_DB_USER/$SPARK_DB_PASSWORD << EOF >> $RANGERHOME/LOG/migration_spark_21_01_2013.log
--whenever sqlerror exit 5 ;
--whenever oserror exit 6 ;
set feedback on ;
set serveroutput on ;

DROP TABLE TEM_NULLTRAFFIC_EXCLUDE;
CREATE TABLE TEM_NULLTRAFFIC_EXCLUDE(
        ID              NUMBER(10),
        CALLED_NUMBER   VARCHAR2(80),
        PRIMARY KEY (ID,CALLED_NUMBER)
);

DELETE FROM TABLE_COLUMN where TBD_ID = (select TBD_ID from TABLE_DFN where TBD_NAME='DS_SUBSCRIBER_V') and TCL_NAME='IS_HYBRID';
INSERT INTO TABLE_COLUMN
         (TCL_ID,TBD_ID,TCL_NAME,TCL_DISPLAY,TCL_ORDER_NO,TCL_PRIMARY_KEY_FL,TCL_TYPE,TCL_LENGTH,TCL_MANDATORY_FL,TCL_DELETE_FL,TCL_VERSION_ID,PTN_ID)
                 VALUES(tbl_clmn_seq.nextval,(select TBD_ID from TABLE_DFN where TBD_NAME='DS_SUBSCRIBER_V'),'IS_HYBRID','IS_HYBRID',18,'N','string','10','N','N',1,1);

DELETE FROM TABLE_COLUMN WHERE TBD_ID = (select TBD_ID from TABLE_DFN where TBD_NAME = 'TEM_NULLTRAFFIC_EXCLUDE');

DELETE FROM TABLE_INST WHERE TBD_ID = (select TBD_ID from TABLE_DFN where TBD_NAME = 'TEM_NULLTRAFFIC_EXCLUDE');

DELETE FROM TABLE_DFN WHERE TBD_NAME = 'TEM_NULLTRAFFIC_EXCLUDE';

select  'TEM_NULLTRAFFIC_EXCLUDE' from dual;
-------TEM_NULLTRAFFIC_EXCLUDE-----
INSERT INTO TABLE_DFN
        (TBD_ID,SCT_ID,TBD_NAME,TBD_PREFIX,TBD_SYSTEM_FL,TBD_INTERNAL_FL,TBD_TEMP_FL,TBD_DELETE_FL,TBD_VERSION_ID,PTN_ID )
         VALUES( tbl_dfn_seq.nextval,1,'TEM_NULLTRAFFIC_EXCLUDE','TEM_NULLTRAFFIC_EXCLUDE','N','N','N','N',1,1);
INSERT INTO TABLE_INST
        (TIN_ID,TBD_ID,SCH_ID,TIN_TABLE_NAME,TIN_DISPLAY_NAME,TIN_ALIAS,TIN_DELETE_FL,TIN_VERSION_ID,PTN_ID)
        VALUES(tbl_inst_seq.nextval,(select TBD_ID from TABLE_DFN where TBD_NAME='TEM_NULLTRAFFIC_EXCLUDE'),1,'TEM_NULLTRAFFIC_EXCLUDE','TEM_NULLTRAFFIC_EXCLUDE',
        'TEM_NULLTRAFFIC_EXCLUDE','N',1,1);
INSERT INTO TABLE_COLUMN
              (TCL_ID,TBD_ID,TCL_NAME,TCL_DISPLAY,TCL_ORDER_NO,TCL_PRIMARY_KEY_FL,TCL_TYPE,TCL_LENGTH,TCL_MANDATORY_FL,TCL_DELETE_FL,TCL_VERSION_ID,PTN_ID)
              VALUES(tbl_clmn_seq.nextval,(select TBD_ID from TABLE_DFN where TBD_NAME='TEM_NULLTRAFFIC_EXCLUDE'),'ID','ID',1,'Y','int',5,'Y','N',1,1);
INSERT INTO TABLE_COLUMN
              (TCL_ID,TBD_ID,TCL_NAME,TCL_DISPLAY,TCL_ORDER_NO,TCL_PRIMARY_KEY_FL,TCL_TYPE,TCL_LENGTH,TCL_MANDATORY_FL,TCL_DELETE_FL,TCL_VERSION_ID,PTN_ID)
              VALUES(tbl_clmn_seq.nextval,(select TBD_ID from TABLE_DFN where TBD_NAME='TEM_NULLTRAFFIC_EXCLUDE'),'CALLED_NUMBER','CALLED_NUMBER',2,'Y','string',80,'Y','N',1,1);
commit ;
quit ;
EOF

if [ $? -eq 0 ]
then
    echo -e "\n Spark Migration Completed !!!"
    echo -e "Check Log : $RANGERHOME/LOG/migration_spark_21_01_2013.log\n"
else
    echo -e "\n Spark Migration Exited with Error !!!"
    echo -e "Check Log : $RANGERHOME/LOG/migration_spark_21_01_2013.log\n"
fi ;

