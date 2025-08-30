declare 
  -- Local variables here
  i integer;
begin
  -- Test statements here
  select count(*)
    into i
    from user_tables
   where table_name like 'TABLESPACE_USAGE';
  if i > 0 then
    execute immediate 'drop table TABLESPACE_USAGE';
    execute immediate 'create table TABLESPACE_USAGE
              (
                tablespace_name     VARCHAR2(255),
                total_allocated_mb  NUMBER,
                total_phys_alloc_mb NUMBER,
                used_mb             NUMBER,
                percentage_used     NUMBER,
		is_recent		    varchar(10),
                run_date            DATE
              )';
   else
		    execute immediate 'create table TABLESPACE_USAGE
              (
                tablespace_name     VARCHAR2(255),
                total_allocated_mb  NUMBER,
                total_phys_alloc_mb NUMBER,
                used_mb             NUMBER,
                percentage_used     NUMBER,
		is_recent		    varchar(10),
                run_date            DATE
              )';
  end if;
  
   select count(*)
    into i
    from user_tables
   where table_name like 'INDEX_DETAILS';
  if i > 0 then
              execute immediate 'drop table INDEX_DETAILS';      
                execute immediate 'create table INDEX_DETAILS
            (
              index_name      VARCHAR2(255),
              status          VARCHAR2(255),
		table_name	varchar2(255),
		partition_name varchar2(255),
		is_recent     varchar(10),
              run_date        DATE
            )';
	else
                   execute immediate 'create table INDEX_DETAILS
            (
              index_name      VARCHAR2(255),
              status          VARCHAR2(255),
		table_name	varchar2(255),
		partition_name varchar2(255),
		is_recent     varchar(10),
              run_date        DATE
            )';
  end if;
  
    select count(*)
    into i
    from user_tables
   where table_name like 'FRAGMENTATION_DETAILS';
  if i > 0 then
              execute immediate 'drop table FRAGMENTATION_DETAILS';      
                execute immediate 'create table FRAGMENTATION_DETAILS
                            (
                              table_name         VARCHAR2(255),
                              size_GB         NUMBER,
                              actual_data_GB  NUMBER,
                              wasted_space_GB NUMBER,
				is_recent		    varchar(10),
                              Run_Date         DATE
                            )';
	else
                execute immediate 'create table FRAGMENTATION_DETAILS
                            (
                              table_name         VARCHAR2(255),
                              size_GB         NUMBER,
                              actual_data_GB  NUMBER,
                              wasted_space_GB NUMBER,
		              is_recent     varchar(10),
                              Run_Date         DATE
                            )';	
  end if;
  
   select count(*)
    into i
    from user_tables
   where table_name like 'TABLE_LIST';
  if i > 0 then
              execute immediate 'drop table TABLE_LIST';      
                execute immediate 'create table TABLE_LIST
                                    (
                                      table_name             VARCHAR2(255),
                                      tablespace_name        VARCHAR2(255),
                                      last_analyzed          DATE,
                                      Days_Before_Analyzed NUMBER,
				      		is_recent		    varchar(10),
                                      Run_Date             DATE
                                    )';
	else
                execute immediate 'create table TABLE_LIST
                                    (
                                      table_name             VARCHAR2(255),
                                      tablespace_name        VARCHAR2(255),
                                      last_analyzed          DATE,
                                      Days_Before_Analyzed NUMBER,
				      		is_recent		    varchar(10),
                                      Run_Date             DATE
                                    )';	
  end if;
  
     select count(*)
    into i
    from user_tables
   where table_name like 'UNUSED_TABLES';
  if i > 0 then
              execute immediate 'drop table UNUSED_TABLES';     
              execute immediate 'create table UNUSED_TABLES (table_name varchar2(255),		is_recent		    varchar(10),run_date date)';
	else
			  execute immediate 'create table UNUSED_TABLES (table_name varchar2(255),		is_recent		    varchar(10),run_date date)';	
  end if;

 select count(*)
    into i
    from user_tables
   where table_name like 'APP_TBL_LIST';
  if i > 0 then
              execute immediate 'drop table APP_TBL_LIST';     
              execute immediate 'create table APP_TBL_LIST (table_name varchar2(255))';
	else
			  execute immediate 'create table APP_TBL_LIST (table_name varchar2(255))';	
  end if;



  
end;
/
