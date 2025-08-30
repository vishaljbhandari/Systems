$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever oserror exit 5;
whenever sqlerror exit 5;

delete from custom_nickname_values ;
delete from custom_nicknames ;
delete from list_details ;
delete from list_configs_networks ;
delete from list_configs ;
delete from scheduler_command_maps ;

commit;
quit;
EOF
if test $? -eq 5
then
	exit 1
fi

exit 0
	

