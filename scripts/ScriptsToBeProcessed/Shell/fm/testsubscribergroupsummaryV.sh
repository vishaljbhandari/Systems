$SQL_COMMAND /nolog <<EOF
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
set heading off
spool testsubscriber_group_summaryV.dat

select 1 from dual where 14 = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1025 group by network_id);
select 1 from dual where 11 = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1026 group by network_id);
select 1 from dual where 4  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1027 group by network_id);

select 1 from dual where 2  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1025 and group_name ='Default' group by network_id);
select 1 from dual where 1  = (select sum(group_count) from subscriber_groups_summary where record_config_id =4 and network_id = 1025 and group_name ='Default' group by network_id);

select 1 from dual where 3  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1025 and group_name ='PVN' group by network_id);
select 1 from dual where 2  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1026 and group_name ='PVN' group by network_id);
select 1 from dual where 1  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1027 and group_name ='PVN' group by network_id);

select 1 from dual where 2  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1025 and group_name ='AGN' group by network_id);
select 1 from dual where 3  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1026 and group_name ='AGN' group by network_id);
select 1 from dual where 1  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1027 and group_name ='AGN' group by network_id);

select 1 from dual where 2  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1025 and group_name ='CAN' group by network_id);
select 1 from dual where 2  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1026 and group_name ='CAN' group by network_id);
select 1 from dual where 1  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1027 and group_name ='CAN' group by network_id);

select 1 from dual where 3  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1025 and group_name ='BAN' group by network_id);
select 1 from dual where 2  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1026 and group_name ='BAN' group by network_id);
select 1 from dual where 0  = (select sum(group_count) from subscriber_groups_summary where record_config_id =3 and network_id = 1027 and group_name ='BAN' group by network_id);

delete from account where id = 1700;
delete from subscriber where id > 4;
delete from subscriber_groups_summary ;

spool off
EOF

sqlret=$?

if [ "$sqlret" -ne "0" ] || grep "no rows" testsubscriber_group_summaryV.dat 
then
	exitval=1
else
	exitval=0
fi

rm -f testsubscriber_group_summaryV.dat
exit $exitval
