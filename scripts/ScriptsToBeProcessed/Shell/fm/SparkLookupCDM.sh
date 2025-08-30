#!/bin/bash


NikiraDBUser=$1
NikiraDBPassword=$2
NikiraOracleSid=$3
SparkDBUser=$4
SparkDBPasswd=$5
SparkOracleSid=$6
SparkDbLink=$7
FMDBLink=$8


sqlplus -s  $SparkDBUser/$SparkDBPasswd@$SparkOracleSid << EOF


--ACCOUNT

@spark_table_inst_ACCOUNT.sql

--SUBSCRIBER

@spark_table_inst_SUBSCRIBER.sql 


--------UNF_CONVERSION

delete from table_column where tbd_id = (select tbd_id from table_dfn where tbd_name ='UNF_CONVERSION' );
delete from table_dfn where tbd_name = 'UNF_CONVERSION';
delete from table_inst where tin_table_name = 'UNF_CONVERSION';



insert into table_dfn
			(tbd_id,sct_id,tbd_name,tbd_prefix,tbd_system_fl,tbd_internal_fl,tbd_temp_fl,tbd_delete_fl,tbd_hybrid_fl,tbd_version_id,ptn_id,system_generated_fl)
	       values((select NOI_CURRENT_NO + 1 as id from NEXT_OBJECT_ID where NOI_OBJECT_NAME='TableDfn'),1,'UNF_CONVERSION','UNF_CONVERSION','N','N','N','N','N',1,1,'N'
			);
		   
insert into table_inst
			(tin_id,tbd_id,SCH_ID,tin_table_name,tin_display_name,tin_alias,tin_delete_fl,tin_version_id,ptn_id,system_generated_fl)
	        values((select NOI_CURRENT_NO + 1 as id from NEXT_OBJECT_ID where NOI_OBJECT_NAME='TableInst'),(select tbd_id from table_dfn where tbd_name = 'UNF_CONVERSION'), 2,'UNF_CONVERSION','UNF_CONVERSION','UNF_CONVERSION','N',1,1,'N'
			);


insert into table_column
              (tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
	              values((select NOI_CURRENT_NO + 1 as id from NEXT_OBJECT_ID where NOI_OBJECT_NAME='TableColumn'),(select tbd_id from table_dfn where tbd_name = 'UNF_CONVERSION'),'UNF_ID','UNF_ID',1,'Y','int',10,'Y','N',1,1
					          );

insert into table_column
				(tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
				values((select NOI_CURRENT_NO + 2 as id from NEXT_OBJECT_ID where NOI_OBJECT_NAME='TableColumn'),(select tbd_id from table_dfn where tbd_name = 'UNF_CONVERSION'),'PREFIX_STRING','PREFIX_STRING',2,'N','string',20,'Y','N',1,1
				);
	
insert into table_column
				(tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
				values((select NOI_CURRENT_NO + 3 as id from NEXT_OBJECT_ID where NOI_OBJECT_NAME='TableColumn'),(select tbd_id from table_dfn where tbd_name = 'UNF_CONVERSION'),'REPLACE_STRING','REPLACE_STRING',3,'N','string',20,'Y','N',1,1
				);

insert into table_column
				(tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
				values((select NOI_CURRENT_NO + 4 as id from NEXT_OBJECT_ID where NOI_OBJECT_NAME='TableColumn'),(select tbd_id from table_dfn where tbd_name = 'UNF_CONVERSION'),'DS_NAME','DS_NAME',4,'N','string',15,'Y','N',1,1
				);


update next_object_id set NOI_CURRENT_NO = (select tbd_id + 1 from table_dfn where tbd_name = 'UNF_CONVERSION') where NOI_OBJECT_NAME = 'TableDfn';
update next_object_id set NOI_CURRENT_NO = ((select max(TCL_ID)+1 from table_column))  where NOI_OBJECT_NAME = 'TableColumn';

update next_object_id set NOI_CURRENT_NO = (select tin_id +1 from table_inst where TIN_TABLE_NAME ='UNF_CONVERSION') where NOI_OBJECT_NAME = 'TableInst';






			  
----COUNTRY CODE change

delete from table_column where tbd_id = (select tbd_id from table_dfn where tbd_name ='CDM_COUNTRY_CODES' );
delete from table_dfn where tbd_name = 'CDM_COUNTRY_CODES';
delete from table_inst where tin_table_name = 'CDM_COUNTRY_CODES';


INSERT INTO 
TABLE_DFN(TBD_ID,SCT_ID,TBD_NAME,TBD_PREFIX,TBD_SYSTEM_FL,TBD_INTERNAL_FL,TBD_TEMP_FL,TBD_HYBRID_FL,TBD_DELETE_FL,TBD_VERSION_ID,PTN_ID,SYSTEM_GENERATED_FL )
	VALUES((select NOI_CURRENT_NO + 1 as id from NEXT_OBJECT_ID where NOI_OBJECT_NAME='TableDfn'),1,'CDM_COUNTRY_CODES','CDM_COUNTRY_CODES','N','N','N','N','N',1,1,'N');
INSERT INTO
	TABLE_INST(TIN_ID,TBD_ID,SCH_ID,TIN_TABLE_NAME,TIN_DISPLAY_NAME,TIN_ALIAS,TIN_DELETE_FL,TIN_VERSION_ID,PTN_ID,SYSTEM_GENERATED_FL)
	 VALUES((select NOI_CURRENT_NO + 1 as id from NEXT_OBJECT_ID where NOI_OBJECT_NAME='TableInst'),(select tbd_id from table_dfn where tbd_name = 'CDM_COUNTRY_CODES'),
			 1,'CDM_COUNTRY_CODES','CDM_COUNTRY_CODES','CDM_COUNTRY_CODES','N',1,1,'N');
INSERT INTO
	 TABLE_COLUMN(TCL_ID,TBD_ID,TCL_NAME,TCL_DISPLAY,TCL_ORDER_NO,TCL_PRIMARY_KEY_FL,TCL_TYPE,TCL_LENGTH,TCL_MANDATORY_FL,TCL_DELETE_FL,TCL_VERSION_ID,PTN_ID)
	VALUES(((select NOI_CURRENT_NO + 1 as id from NEXT_OBJECT_ID where NOI_OBJECT_NAME='TableColumn')),(select tbd_id from table_dfn where tbd_name = 'CDM_COUNTRY_CODES'),'ID','ID',1,'Y','int',0,'Y','N',1,1);
INSERT INTO
	TABLE_COLUMN(TCL_ID,TBD_ID,TCL_NAME,TCL_DISPLAY,TCL_ORDER_NO,TCL_PRIMARY_KEY_FL,TCL_TYPE,TCL_LENGTH,TCL_MANDATORY_FL,TCL_DELETE_FL,TCL_VERSION_ID,PTN_ID)
	VALUES(((select NOI_CURRENT_NO + 2 as id from NEXT_OBJECT_ID where NOI_OBJECT_NAME='TableColumn')),(select tbd_id from table_dfn where tbd_name = 'CDM_COUNTRY_CODES'),'CODE','CODE',2,'N','string',20,'Y','N',1,1);

update next_object_id set NOI_CURRENT_NO = (select tbd_id + 1 from table_dfn where tbd_name = 'CDM_COUNTRY_CODES') where NOI_OBJECT_NAME = 'TableDfn';
update next_object_id set NOI_CURRENT_NO = ((select max(TCL_ID)+1 from table_column))  where NOI_OBJECT_NAME = 'TableColumn';

update next_object_id set NOI_CURRENT_NO = (select tin_id +1 from table_inst where TIN_TABLE_NAME ='CDM_COUNTRY_CODES') where NOI_OBJECT_NAME = 'TableInst';



create or replace synonym CDM_COUNTRY_CODES for $NikiraDBUser.CDM_COUNTRY_CODES@$FMDBLink;


commit;
quit;
EOF
