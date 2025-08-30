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
	echo "Migration started from SP1 to SP2"
	sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >>Migration_SP1_SP2.log
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
	sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >>Migration_SP1_SP2.log
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

function Migration_SP1_SP2
{ 
   sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >>Migration_SP1_SP2.log
        set heading on ;
        set feedback on ;
        set serveroutput on ;
        whenever sqlerror exit 5 rollback ;
        whenever oserror exit 5 ;
        @TaskMetaTable_SP2.sql;
		@SP1_to_SP2_DDL_NEW_TABLES.sql;
		@sequence_new_tables.sql;
		@SP1_to_SP2_DML_NEW_TABLES.sql;
        DECLARE 
           cursor task_SP2 is select * from tasks_meta_sp2 ;
           ROW_COUNT NUMBER(10) := 0;
            
        BEGIN
            for tk in task_SP2
                loop 
					if tk.LINK is NULL then
						select count(*) into ROW_COUNT from tasks where PARENT_ID = tk.PARENT_ID and NAME = tk.NAME and LINK is null;
					else
						select count(*) into ROW_COUNT from tasks where PARENT_ID = tk.PARENT_ID and NAME = tk.NAME and LINK = tk.LINK;
					end if;

                    if ROW_COUNT = 0 THEN
						dbms_output.put_line('task id is '||tk.ID||' the task name is '||tk.name);
					    insert into tasks values(tk.ID,tk.PARENT_ID,tk.NAME,tk.LINK,tk.IS_DEFAULT,tk.IS_MENU_ITEM,tk.PACKAGE_IDS,tk.IMAGE_SRC,tk.IS_DROPDOWN_LINK);
						for rg_id in (select * from ranger_groups where name in ('nadmin','radmin','default','InternalUser'))
						loop
							if rg_id.name <> 'InternalUser' then
								insert into ranger_groups_tasks values (rg_id.id, tk.ID);
							else 
								if tk.name in ('Entities','Events','Alarms','Alerts','Standard Rules','Smart Patterns','Statistical Rules','eFingerprinting','User Management','Search') then
									insert into ranger_groups_tasks values (rg_id.id, tk.id);
								end if;
							end if;
						end loop;
					else
						if tk.LINK is NULL then
							update tasks set IS_DROPDOWN_LINK = tk.IS_DROPDOWN_LINK where PARENT_ID = tk.PARENT_ID and NAME = tk.NAME and LINK is null;
						else
							update tasks set IS_DROPDOWN_LINK = tk.IS_DROPDOWN_LINK where PARENT_ID = tk.PARENT_ID and NAME = tk.NAME and LINK = tk.LINK;
						end if;
					end if;
                end loop;
				delete from ranger_groups_tasks where task_id = 1153 and ranger_group_id = 0;
				execute immediate 'drop table tasks_meta_sp2';
				execute immediate 'drop table tasks_meta';
				execute immediate 'drop table tasks_meta_2';
                @SP1_to_SP2_DML.sql;
				update CONFIGURATIONS set value = '$PREVEAROOT/data/subscriber/' where CONFIG_KEY = 'PREVEA_OUTPUT_DIRECTORY';
				update CONFIGURATIONS set value = '$RANGERHOME/bin/roc_control_panel_cookiefile' where CONFIG_KEY = 'ROC_CONTROL_PANEL.COOKIE_FILE';
				commit;
				update tasks set IS_DROPDOWN_LINK = 1 where parent_id = (select id from tasks where name = 'Events' and parent_id = 60 and link is null);
				@SP1_to_SP2_DML_1.sql;
				commit;
		END;
/
EOF
}

function UpdateTasks_2
{ 
   sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >>Migration_SP1_SP2.log
        set heading on ;
        set feedback on ;
        set serveroutput on ;
        whenever sqlerror exit 5 rollback ;
		whenever oserror exit 5 ;
		create or replace procedure set_dropdown_task(n_task_id number)                             
		as
		link_name varchar2(1000) ;
		begin       
			select link into link_name from tasks where id = n_task_id;
			if link_name is not null then 
				update tasks set parent_id = (select id from tasks where name = 'Events' and parent_id = 60 and link is null), is_dropdown_link = 1 where id = n_task_id ;
			else           
				delete from RANGER_GROUPS_TASKS where task_id = n_task_id;
				update tasks set IS_MENU_ITEM = 0 where id = n_task_id;
				for i in (select id from tasks where parent_id = n_task_id)
				loop
					set_dropdown_task(i.id) ;
				end loop;
			end if;        
		end set_dropdown_task;             
		/

		begin              
			for tk1 in (select id, link from tasks where parent_id = (select id from tasks where name = 'Events' and parent_id = 60 and link is null))
			loop
				if tk1.link is not null then
					update tasks set is_dropdown_link = 1 where id = tk1.id ;	
				else
					set_dropdown_task(tk1.id);
				end if;
			end loop;
		end;
		/               

EOF
}

function RunProcedures
{
	sed "s#@common_mount_point@#$COMMON_MOUNT_POINT#g" gdo_packages_definitions.sql.parse.in > gdo_packages_definitions.sql
   sqlplus -s $Nikira_DB_User/$Nikira_DB_Password@$Nikira_Oracle_SID << EOF >>Migration_SP1_SP2.log
        set heading on ;
        set feedback on ;
        set serveroutput on ;
        whenever sqlerror exit 5 rollback ;
        whenever oserror exit 5 ;
		@gdo_procedure_definitions.sql;
		@dsm_view_definitions.sql;
	   	@gdo_packages_definitions.sql;
		commit;
/
EOF
}

function Main()
{
	CheckParam
	UpdateTasks
	UpdateTasks_1
	Migration_SP1_SP2    
	UpdateTasks_2
	RunProcedures
}
Main
