set serveroutput on ;
set feedback on ;
declare
	Cleanup_Record_Ids            varchar2(1024) ;
	TableName                     varchar2(256) ;
	Record_Config_Options         varchar2(64) ;
	Last_Record_Processed_Date    date ;
	start_pos                     number := 1 ;
	curr_pos                      number ;
	delimeter                     varchar2(2) := ',' ;
	RetentionInterval             NUMBER(10) ;
	Cleanup_Interval              varchar2(64) ;
	end_cleanup_date                        date ;
	start_cleanup_date                      date ;
	
	cursor record_ids is select distinct id from record_configs where instr(',' || Cleanup_Record_Ids || ',', ',' || id || ',') > 0 ;
begin
	select value into Cleanup_Record_Ids from configurations where config_key = 'CLEANUP.RECORDS.OPTION' ;
	for recordconfig in record_ids
	loop
		select tname into TableName from record_configs where id = recordconfig.id ;
		select value into Record_Config_Options from configurations
			where config_key = 'CLEANUP.RECORDS.'||TableName||'.OPTIONS' ;
		begin
			tokenizer (start_pos, delimeter, Record_Config_Options, Cleanup_Interval, curr_pos) ;
			RetentionInterval := to_number(Cleanup_Interval) ;
			IF ( ( RetentionInterval <= 0 and RetentionInterval <> -1 ) OR RetentionInterval IS NULL) THEN
				raise INVALID_NUMBER ;
			END IF ;
		EXCEPTION
		when others then
			dbms_output.put_line('Failed to fetch cleanup interval for record config is :'||recordconfig.id) ;
		end ;

		if RetentionInterval = 90 then
			Last_Record_Processed_Date := Utility.GETCURRENTPARTITIONDATE(recordconfig.id) ;
			tokenizer (start_pos, delimeter, Record_Config_Options, Cleanup_Interval, curr_pos) ;
			end_cleanup_date := Last_Record_Processed_Date - 88 ;

			 update configurations set value = to_char(end_cleanup_date,'yyyy/mm/dd') where config_key ='CLEANUP.RECORDS.'||'LAST_'||TableName||'_DAY_TRUNCED' ;
			 update configurations set value=replace(value,',90',',85') where config_key = 'CLEANUP.RECORDS.'||TableName||'.OPTIONS' ;

		end if ;
	end loop ;
exception
when others then
	dbms_output.put_line(sqlerrm) ;
end;
/
commit;
