#!/bin/bash
spool spark.log

sqlplus -s  spark_p/spark_p@ora10g << EOF

--max table id is  374
delete from table_dfn where tbd_id between 1200 and 1300;
delete from table_inst where tbd_id between 1200 and 1300;
delete from table_column where tbd_id between 1200 and 1300;

--ACCOUNT
insert into table_dfn(tbd_id,sct_id,tbd_name,tbd_prefix,tbd_system_fl,TBD_INTERNAL_FL,TBD_TEMP_FL,tbd_delete_fl,tbd_version_id,ptn_id )
            values(1252,1,'ACCOUNT','ACCOUNT','N','N','N','N',1,1
            );


insert into table_inst(tin_id,tbd_id,SCH_ID,tin_table_name,tin_display_name,tin_alias,tin_delete_fl,tin_version_id,ptn_id)
            values( 1252,1252, 2,'ACCOUNT','ACCOUNT','ACCOUNT','N',1,1
            );


insert into table_column(
		tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
            values(12075,1252,'ID','ID',1,'N','int',0,'Y','N',1,1
            );

insert into table_column(
		tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
            values(12076,1252,'RATE_PLAN','RATE_PLAN',2,'N','string',0,'Y','N',1,1
            );

insert into table_column(
		tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
            values(12665,1252,'ACCOUNT_TYPE','ACCOUNT_TYPE',3,'N','int',0,'Y','N',1,1);

--SUBSCRIBER

insert into table_dfn(tbd_id,sct_id,tbd_name,tbd_prefix,tbd_system_fl,TBD_INTERNAL_FL,TBD_TEMP_FL,tbd_delete_fl,tbd_version_id,ptn_id )
            values(1258,1,'NIK_SUBSCRIBER','NIK_SUBSCRIBER','N','N','N','N',1,1
            );

insert into table_inst(tin_id,tbd_id,SCH_ID,tin_table_name,tin_display_name,tin_alias,tin_delete_fl,tin_version_id,ptn_id)
            values(1258,1258, 2,'NIK_SUBSCRIBER','NIK_SUBSCRIBER','NIK_SUBSCRIBER','N',1,1
            );


insert into table_column(
		tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
            values(12087,1258,'ID','ID',1,'N','int',20,'Y','N',1,1
            );

insert into table_column(
		tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
            values(12088,1258,'PHONE_NUMBER','PHONE_NUMBER',2,'N','string',40,'Y','N',1,1
            );
			
insert into table_column(
		tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
            values(12089,1258,'IMSI','IMSI',3,'N','string',20,'Y','N',1,1
            );

insert into table_column(
		tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
            values(12091,1258,'ACCOUNT_ID','ACCOUNT_ID',4,'N','int',20,'Y','N',1,1
            );

insert into table_column(
		tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
            values(12092,1258,'STATUS','STATUS',5,'N','int',20,'Y','N',1,1
            );

insert into table_column(
		tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
			values(12093,1258,'SUBSCRIBER_TYPE','SUBSCRIBER_TYPE',6,'N','int',20,'Y','N',1,1
			);

--------UNF_CONVERSION

insert into table_dfn
        (tbd_id,sct_id,tbd_name,tbd_prefix,tbd_system_fl,TBD_INTERNAL_FL,TBD_TEMP_FL,tbd_delete_fl,tbd_version_id,ptn_id )
         values(1259,1,'UNF_CONVERSION','UNF_CONVERSION','N','N','N','N',1,1
        );

insert into table_inst
        (tin_id,tbd_id,SCH_ID,tin_table_name,tin_display_name,tin_alias,tin_delete_fl,tin_version_id,ptn_id)
        values( 1259,1259, 2,'UNF_CONVERSION','UNF_CONVERSION','UNF_CONVERSION','N',1,1
        );

insert into table_column
              (tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
              values(12094,1259,'UNF_ID','UNF_ID',1,'Y','int',10,'Y','N',1,1
        );

insert into table_column
              (tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
              values(12095,1259,'PREFIX_STRING','PREFIX_STRING',2,'N','string',20,'Y','N',1,1
        );

insert into table_column
              (tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
              values(12096,1259,'REPLACE_STRING','REPLACE_STRING',3,'N','string',20,'Y','N',1,1
        );



insert into table_column
              (tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
              values(12097,1259,'DS_NAME','DS_NAME',4,'N','string',15,'Y','N',1,1
        );
----subscriber
insert into table_column(
        tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
            values(12098,1258,'PRODUCT_TYPE','PRODUCT_TYPE',7,'N','int',20,'Y','N',1,1
            );
insert into table_column(
        tcl_id,tbd_id,tcl_name,tcl_display,tcl_order_no,tcl_primary_key_fl,tcl_type,tcl_length,tcl_mandatory_fl,tcl_delete_fl,tcl_version_id,ptn_id)
            values(12099,1258,'MCN1','MCN1',8,'N','string',20,'Y','N',1,1
            );
        commit;
        quit;
EOF

