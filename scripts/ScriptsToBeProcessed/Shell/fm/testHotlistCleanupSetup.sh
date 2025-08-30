$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;


delete from hotlist_groups_suspect_values ;
delete from suspect_values ;
delete from hotlist_groups_networks ;
delete from hotlist_groups ;
insert into hotlist_groups (id, name, description, is_active)
			values (HOTLIST_GROUPS_SEQ.nextval, 'Suspended', 'Suspended', 1) ;

insert into suspect_values (id, value, source, reason, user_id, entity_type, expiry_date)  
			values (SUSPECT_VALUES_SEQ.nextval, '+9189', 'test', '', 0, 2, sysdate -2) ;

insert into hotlist_groups_suspect_values (hotlist_group_id, suspect_value_id)
			values (HOTLIST_GROUPS_SEQ.currval, SUSPECT_VALUES_SEQ.currval) ;

insert into suspect_values (id, value, source, reason, user_id, entity_type, expiry_date)  
			values (SUSPECT_VALUES_SEQ.nextval, '+9189', 'test', '', 0, 2, sysdate -3) ;

insert into hotlist_groups_suspect_values (hotlist_group_id, suspect_value_id)
			values (HOTLIST_GROUPS_SEQ.currval, SUSPECT_VALUES_SEQ.currval) ;

insert into suspect_values (id, value, source, reason, user_id, entity_type, expiry_date)  
			values (SUSPECT_VALUES_SEQ.nextval, '+9189', 'test', '', 0, 2, sysdate) ;

insert into hotlist_groups_suspect_values (hotlist_group_id, suspect_value_id)
			values (HOTLIST_GROUPS_SEQ.currval, SUSPECT_VALUES_SEQ.currval) ;

insert into suspect_values (id, value, source, reason, user_id, entity_type, expiry_date)  
			values (SUSPECT_VALUES_SEQ.nextval, '+9189', 'test', '', 0, 2, sysdate + 1) ;

insert into hotlist_groups_suspect_values (hotlist_group_id, suspect_value_id)
			values (HOTLIST_GROUPS_SEQ.currval, SUSPECT_VALUES_SEQ.currval) ;

commit;
quit;
EOF
if test $? -eq 5
then
	exit 1
fi

exit 0
	
