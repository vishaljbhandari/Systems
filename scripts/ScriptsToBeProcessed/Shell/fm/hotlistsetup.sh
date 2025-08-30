#! /bin/bash

ListType=0  
Hotlist='_Hotlist'
ValueType=2
$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
set SERVEROUTPUT on ;

declare
	field_category_name         varchar2(40) ;
	field_category_data_type    number(10) ;
	field_category_id		    number(10) ;
	
	cursor field_categories_cursor is
		SELECT id, name, data_type, category
		FROM field_categories
	    WHERE category like '%NICKNAME%'
		ORDER BY id;
	field_category_record field_categories_cursor%ROWTYPE;

	begin
		open field_categories_cursor ;
		loop
			fetch field_categories_cursor into field_category_record ;
			exit when field_categories_cursor%NOTFOUND;

			field_category_name      := field_category_record.name ;
			field_category_data_type := field_category_record.data_type ;
			field_category_id		 := field_category_record.id ;
			
			INSERT INTO list_configs(id, name, list_type, data_type, field_category_id)
			VALUES (list_configs_seq.nextval, field_category_name||'$Hotlist', $ListType,
				field_category_data_type, field_category_id) ;
			
			INSERT INTO list_details (id, list_config_id, value_type, value)
			SELECT list_details_seq.nextval, list_configs_seq.currval , $ValueType, 
				 'select value from suspect_values where entity_type = '||field_category_id 
			FROM dual ;
			
		end loop ;
	end ;	
/
commit;

EOF
