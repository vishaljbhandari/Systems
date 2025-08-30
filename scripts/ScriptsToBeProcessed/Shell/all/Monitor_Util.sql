create or replace procedure Monitor_util(index_flag   varchar default 1,
                                         tblspace_flag number default 1,
                                         fragement_flag    number default 1,
                                         table_last_analyzed_flag number default 1,
                                         unused_table_flag number default 0,
                                         app_tbl_qry varchar default null,
                                         retention_days number default 60) as


begin
    --clean up the existing data based on retention period.
    delete from index_details where run_date < sysdate - retention_days;
    delete from tablespace_usage where run_date < sysdate - retention_days;
    delete from FRAGMENTATION_DETAILS where run_date < sysdate - retention_days;
    delete from TABLE_LIST where run_date < sysdate - retention_days;
    delete from unused_tables where run_date < sysdate - retention_days;
    commit;

    if index_flag = 1 then
      update INDEX_DETAILS set is_recent='N' where is_recent='Y';
   
      insert into index_details
        select ind.index_name, ind.status, ind.table_name, '' as "partition_name",'Y',sysdate
          from user_indexes ind
         where ind.status = 'UNUSABLE'
        union all
        select part_ind.index_name,
               part_ind.status,
               part_table.table_name,
               part_table.partition_name,'Y',sysdate
          from user_ind_partitions part_ind
         inner join user_tab_partitions part_table
            on part_ind.partition_name = part_table.partition_name
         where part_ind.status = 'UNUSABLE';

       
  end if;
  if tblspace_flag = 1 then
    --tablespace utilization
    update tablespace_usage set is_recent='N'  where is_recent='Y';
    insert into tablespace_usage
      select a.tablespace_name,
             round(a.bytes_alloc / (1024 * 1024),2) ,--"TOTAL ALLOC (MB)"
             round(a.physical_bytes / (1024 * 1024),2) ,--"TOTAL PHYS ALLOC (MB)"
             round((nvl(b.tot_used, 0) / (1024 * 1024)),2) ,--"USED (MB)"
             round((nvl(b.tot_used, 0) / a.bytes_alloc),2) * 100 ,--"% USED"
             'Y',
             sysdate
        from (select tablespace_name,
                     sum(bytes) physical_bytes,
                     sum(decode(autoextensible, 'NO', bytes, 'YES', maxbytes)) bytes_alloc
                from dba_data_files
               group by tablespace_name) a,
             (select tablespace_name, sum(bytes) tot_used
                from user_segments
               group by tablespace_name) b
       where a.tablespace_name = b.tablespace_name(+)
            and   (nvl(b.tot_used,0)/a.bytes_alloc)*100 > 80 --if table space used percentage is more than 80 %
         and a.tablespace_name not in
             (select distinct tablespace_name from dba_temp_files)
         and a.tablespace_name not like 'UNDO%'
       order by 1;
    commit;
  end if;
  if fragement_flag = 1 then
    update FRAGMENTATION_DETAILS set is_recent='N'  where is_recent='Y';
    insert into FRAGMENTATION_DETAILS
      select table_name,
             round(round((blocks * 8), 2) / 1024 / 1024, 2),-- "size(GB)"
             round(round((num_rows * avg_row_len / 1024), 2) / 1024 / 1024,
                   2) ,--"actual_data(GB)",
             round((round((blocks * 8), 2) -
                   round((num_rows * avg_row_len / 1024), 2)) / 1024 / 1024,
                   2) ,--"wasted_space(GB)"
                   'Y',
             sysdate as "Run_Date"
        from user_tables
       where (round((blocks * 8), 2) >
             round((num_rows * avg_row_len / 1024), 2))
             and
             round((round((blocks * 8), 2) -
                   round((num_rows * avg_row_len / 1024), 2)) / 1024 / 1024,
                   2)>0
       ORDER BY 4 DESC;
    commit;
  end if;

    if table_last_analyzed_flag = 1 then
            update TABLE_LIST set is_recent='N'  where is_recent='Y';
            insert into TABLE_LIST
              select table_name,
                     TABLESPACE_NAME,
                     LAST_ANALYZED,
                     round((sysdate - LAST_ANALYZED)),--  "Days Before Analyzed",
                     'Y',
                     sysdate -- Run_Date
                from USER_TABLES
               where (sysdate - LAST_ANALYZED) > 30
                 and temporary = 'N'
               order by round(sysdate - LAST_ANALYZED) desc;
            commit;
    end if;

    if unused_table_flag = 1 then
          update unused_tables set is_recent='N'  where is_recent='Y';
          if app_tbl_qry is not null then
              execute immediate 'insert into APP_TBL_LIST '|| app_tbl_qry;
          end if;
          insert into app_tbl_list values('index_details');
          insert into app_tbl_list values('tablespace_usage');
          insert into app_tbl_list values('FRAGMENTATION_DETAILS');
          insert into app_tbl_list values('TABLE_LIST');
          insert into app_tbl_list values('unused_tables');
          insert into app_tbl_list values('APP_TBL_LIST');

          --DBMS_STATS.FLUSH_DATABASE_MONITORING_INFO;

          insert into unused_tables
            select user_tab.TABLE_NAME,'Y',sysdate from user_tables user_tab where lower(user_tab.table_name) not in(select lower(table_name) from user_TAB_MODIFICATIONS union select lower(table_name) from APP_TBL_LIST) and temporary='N';
          commit;
    end if;
end Monitor_util;
/
