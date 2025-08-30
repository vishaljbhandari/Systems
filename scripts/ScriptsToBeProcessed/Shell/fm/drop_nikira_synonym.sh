if [ $# -ne 2 ]
then
	echo "Usage: ./drop_nikira_synonym.sh <nikira_db_username> <nikira_db_passwd>"
	exit;
fi
sqlplus -s $1/$2 <<EOF
drop synonym prevea_profile_fields;
drop synonym prevea_profile_field_values;
drop synonym prevea_users;
drop synonym prevea_sessions;
drop synonym prevea_user_roles;
drop synonym prevea_roles;

drop synonym prevea_levels;
drop synonym prevea_pf_conditions;
drop synonym prevea_stat_profile_fields;
commit;
EOF
