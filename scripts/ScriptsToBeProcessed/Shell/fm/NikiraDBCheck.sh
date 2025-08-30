#!/bin/bash
$SQL_COMMAND $DB_USER/$DB_PASSWORD$DB_CONNECTION_STRING <<EOF >/dev/null 2>&1
set feedback off;
set heading off;
set lines 120
spool NikiraDB.txt
prompt #               Summary Of Nikira Partitioned DB ;
select '#No Of Tables      ='||count(*) from user_tables;
select '#No Of Indexes     ='||count(*) from user_indexes;
select '#No Of Constraints ='||count(*) from user_constraints where TABLE_NAME not like 'BIN%' ;
select '#No Of Sequences   ='||count(*) from user_sequences;
select '#No Of Pakages     ='||count(distinct OBJECT_NAME) from user_procedures where OBJECT_NAME not like 'BIN%' ;
select '#No Of Procedures  ='||count(distinct PROCEDURE_NAME) from user_procedures ;
select '#No Of Functions   ='||count(*) from  user_objects where OBJECT_TYPE='FUNCTION';
select '#No Of Views       ='||count(*) from user_views ;
select '#No Of Tables Having Partition  ='||nvl(count(Distinct TABLE_NAME),0) from user_tab_partitions;
select '#No Of Table Partitions        ='||nvl(count(Distinct PARTITION_NAME),0) from user_tab_partitions;
select '#No Of Table SubPartitions      ='||nvl (sum(SUBPARTITION_COUNT),0) from user_tab_partitions;
select '#No Of Indexes Having Partition ='||nvl (count(Distinct INDEX_NAME),0) from USER_IND_PARTITIONS;
select '#No Of Index Partitions         ='||nvl (count(Distinct PARTITION_NAME),0) from USER_IND_PARTITIONS;
select '#No Of Index SubPartitions      ='||nvl ((sum(SUBPARTITION_COUNT),0),0) from  USER_IND_PARTITIONS;
prompt #
prompt #

prompt #            Table and TableSpace Details (Tablename,TableSpace Name)
select '#'||TABLE_NAME,TABLESPACE_NAME from user_tables order by TABLE_NAME;
prompt #
prompt #            Index Details (Table Name,Index Name,TableSpace Name)
select '#'||TABLE_NAME,INDEX_NAME,TABLESPACE_NAME from user_indexes order by TABLE_NAME;
prompt #
prompt #            Constraint Details (Table Name,Constraint Name,Constraint Type)
select '#'||TABLE_NAME,CONSTRAINT_NAME,CONSTRAINT_TYPE from user_constraints where TABLE_NAME not like 'BIN%'  order by TABLE_NAME;
prompt #
prompt #            Sequence Details (Sequence Name,Min Value,Incement Value)
select '#'||SEQUENCE_NAME,MIN_VALUE,INCREMENT_BY from user_sequences order by SEQUENCE_NAME;
prompt #
prompt #            Package and Procedure Details (Package Name,Procedure Name)
select '#'||OBJECT_NAME||'        '||PROCEDURE_NAME from user_procedures order by OBJECT_NAME;
prompt #
prompt #            Function Details (Function Name)
select '#'||OBJECT_NAME,SUBOBJECT_NAME from  user_objects where OBJECT_TYPE='FUNCTION' order by OBJECT_NAME;
prompt #
prompt #            View Details (View Name)
select '#'||VIEW_NAME from user_views;
prompt #
prompt #             Table Partition Information (Table Name,Partition Name,Subpartition Count,TableSpace Name)
select '#'||TABLE_NAME,PARTITION_NAME,SUBPARTITION_COUNT,TABLESPACE_NAME from user_tab_partitions where TABLE_NAME not like 'BIN%' order by TABLE_NAME,PARTIT
ION_NAME;
prompt #             Index Partition Information (Indexname,Partition Name,SubPartition Count,TableSpace Name)
select '#'||INDEX_NAME,PARTITION_NAME,SUBPARTITION_COUNT,TABLESPACE_NAME from USER_IND_PARTITIONS;

spool off
EOF
grep "^#" NikiraDB.txt>tmpDB.txt
perl -pi -e 's/^#//g' tmpDB.txt
mv tmpDB.txt NikiraDB.txt
echo "NikiraDB.txt is Generated in the current folder";
