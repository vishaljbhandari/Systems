#!/bin/bash

function CheckParam()
{	
    echo "Checking parameters of DB"
	rm -f -- *.log
	
	if [ -z $DB_USER ]
	then
		echo 'DB_USER Not Set. Please set the environment variable DB_USER'
		exit 1
	fi
	
	if [ -z $DB_PASSWORD ]
	then
		echo 'DB_PASSWORD Not Set. Please set the environment variable DB_PASSWORD'
		exit 1
	fi
	 
	if [ -z $NIKIRA_ORACLE_SID ]
	then
		echo 'NIKIRA_ORACLE_SID Not Set. Please set the environment variable NIKIRA_ORACLE_SID'
		exit 1
	fi
 
	Nikira_DB_User=$DB_USER
	Nikira_DB_Password=$DB_PASSWORD
	Nikira_Oracle_SID=$NIKIRA_ORACLE_SID
	echo "the Nikira_DB_User= $Nikira_DB_User and Nikira_DB_Password=$Nikira_DB_Password" 
}


function UpdateTasks()
{
	echo " now in Update tasks func"
	sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >UpdateTasks.log
   		set heading on ;
   	 	set feedback on ;
   	 	set serveroutput on ;
   	 	whenever sqlerror exit 5 rollback ;
   	 	whenever oserror exit 5 ;
		@TaskMetaTable.sql
   	 	
		DECLARE 
   	 	   cursor task is select * from tasks ;
   	 	   TSK_del_status number(5) := 0;
		   TSK_Merged_to_id number(5) := 0;
           TSK_OLD_ID NUMBER(10) := 0;
		   TSK_NEW_ID NUMBER(10) := 0;
		   TSK_OLD_PARENT_ID NUMBER(10) := 0;
		   TSK_NEW_PARENT_ID NUMBER(10) := 0;
		   TSK_OLD_NAME VARCHAR2(256) := '';
		   TSK_NEW_NAME VARCHAR2(256) := '';
		   TSK_ROLE_ID NUMBER(10) := 0;
		   TSK_CHANGED_PARENT NUMBER(5) := 0;
		   type arrayofnumbers is varray(10) of integer;
		   RANGER_GRP_LIST arrayofnumbers ;
		   merge_id_present number(5) := 0;

		BEGIN
			for tk in task
				loop
				    DBMS_OUTPUT.PUT_LINE('the task name is '|| tk.name);
					select OLD_ID,OLD_PARENT_ID,OLD_NAME,NEW_ID,NEW_PARENT_ID,NEW_NAME,TO_DELETE,MERGED_TO_ID into TSK_OLD_ID,TSK_OLD_PARENT_ID,TSK_OLD_NAME,TSK_NEW_ID,TSK_NEW_PARENT_ID,TSK_NEW_NAME,TSK_del_status,TSK_Merged_to_id from
						tasks_meta where tk.ID = tasks_meta.OLD_ID ;
		   			
					if TSK_del_status = 1 and TSK_Merged_to_id is not null and TSK_OLD_PARENT_ID = 1 and TSK_NEW_PARENT_ID is null 
		   			then
		   				select RANGER_GROUP_ID bulk collect into RANGER_GRP_LIST from ranger_groups_tasks where TASK_ID = tk.id ;
						
						for i in 1..RANGER_GRP_LIST.count LOOP
							select count(*) into merge_id_present from ranger_groups_tasks where TASK_ID = TSK_Merged_to_id and RANGER_GROUP_ID = RANGER_GRP_LIST(i) ;
						
						if merge_id_present = 0
						then	
							insert into ranger_groups_tasks(RANGER_GROUP_ID,TASK_ID) values(RANGER_GRP_LIST(i),TSK_Merged_to_id);
						end if;
						end LOOP;
		   				
						delete from ranger_groups_tasks where TASK_ID = tk.id ;
						update FEATURE_TASK_MAPS set task_id = TSK_Merged_to_id where task_id = tk.id;
						update RECORD_CONFIG_TASK_MAPS set task_id = TSK_Merged_to_id where task_id = tk.id;
						update TASKS_SOURCE set task_id = TSK_Merged_to_id where task_id = tk.id;
           				delete from tasks where id = tk.id;
		   			else
			   			update tasks set ID = TSK_NEW_ID, PARENT_ID = TSK_NEW_PARENT_ID, NAME = TSK_NEW_NAME where ID = tk.id and tk.id = TSK_OLD_ID ;
		   			end if;
				end loop;
				@MigrationOnTopSP2.sql;
		   commit;
		END;   
   	 	
/   	 	
EOF

}

function UpdateTasks_1
{
	sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >UpdateTasks_1.log
   		set heading on ;
   	 	set feedback on ;
   	 	set serveroutput on ;
   	 	whenever sqlerror exit 5 rollback ;
   	 	whenever oserror exit 5 ;

		@TaskMetaTable2.sql 
		
		
		DECLARE 
   	 	   cursor task is select * from tasks ;
           TSK_OLD_ID NUMBER(10) := 0;
		   TSK_NEW_ID NUMBER(10) := 0;
		   TSK_OLD_PARENT_ID NUMBER(10) := 0;
		   TSK_NEW_PARENT_ID NUMBER(10) := 0;
		   TSK_OLD_NAME VARCHAR2(256) := '';
		   TSK_NEW_NAME VARCHAR2(256) := '';
		
			
		BEGIN
			for tk in task
				loop
                    select OLD_ID,OLD_NAME,OLD_PARENT_ID,NEW_ID,NEW_NAME,NEW_PARENT_ID into TSK_OLD_ID,TSK_OLD_NAME,TSK_OLD_PARENT_ID,TSK_NEW_ID,TSK_NEW_NAME,TSK_NEW_PARENT_ID from tasks_meta_2 where tk.ID = tasks_meta_2.OLD_ID ;
   					
			   		update tasks set ID = TSK_NEW_ID, PARENT_ID = TSK_NEW_PARENT_ID, NAME = TSK_NEW_NAME where ID = tk.id and tk.id = TSK_OLD_ID ;
					
				end loop;
		   @Migration_2.sql; 	
		   @Migration_3.sql; 	
		   @Migration_4.sql; 	
		   @Migration_5.sql;
		   @Migration_6.sql;
		   @Migration_7.sql;
		   commit;
		END;   
   	 	
/   	 	
EOF

}


function Main()
{
	CheckParam
	UpdateTasks
	UpdateTasks_1
}
Main
