#! /bin/bash

sqlplus -s $DB_USER/$DB_PASSWORD <<EOF

delete time_zone_rate_types ;
delete org_dest_links ;
delete rater_country_codes ;
delete rate_plans ;
delete networks_rater_special_numbers ;
delete rater_special_numbers ;
delete rate_per_calls ;
delete zone_codes ;
delete rate_types ;
delete default_rates ;
delete partial_cdr_info ;
delete friends_family ;
delete NETWORKS_SDR_RATES ;
delete SDR_RATES;

insert into zone_codes values (1, 'Zone1' ,'Testing') ;
insert into zone_codes values (2, 'Zone2' ,'Testing') ;
insert into zone_codes values (3, 'Default', 'Default') ;
insert into rate_types values (1, 'AllDay', '27*7') ;
insert into time_zone_rate_types values (1, 'AD', to_date('00:00', 'hh24:mi'), to_date('23:59', 'hh24:mi'), 'GS', 'Y', 'AllDay') ;
insert into time_zone_rate_types values (2, 'AD', to_date('00:00', 'hh24:mi'), to_date('23:59', 'hh24:mi'), 'TF', 'Y', 'AllDay') ;
insert into rater_country_codes values  (1,'+967','+967');
insert into rater_country_codes values  (2,'+966','+966');

  
insert into org_dest_links values (1, '+967', '+967', 'Zone1', 'Y', '+967');
insert into org_dest_links values (2, '+967', '+966', 'Zone2', 'Y', '+967');

insert into rate_plans values (1, 'RatePlan', 'N', 'RatePlan', 'GS', 'GSM', 'Zone1', 'AllDay', 'V', 'OF', 0,0, 60, 1, 0,0,0,0,0,0,60,0.5,0,0,0,0,0,1,'HPMN') ;
insert into rate_plans values (2, 'RatePlan', 'N', 'RatePlan', 'TF', 'GSM', 'Zone1', 'AllDay', 'V', 'OF', 0,0, 60, 2, 0,0,0,0,0,0,60,0.5,0,0,0,0,0,1,'HPMN') ;

insert into rate_plans values (3, 'RatePlan', 'N', 'RatePlan1', 'GS', 'GSM', 'Zone1', 'AllDay', 'V', 'OF', 60,5, 1, 1, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,'HPMN') ;
insert into rate_plans values (4, 'RatePlan', 'N', 'RatePlan1', 'TF', 'GSM', 'Zone1', 'AllDay', 'V', 'OF', 120,8, 1, 1, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,'HPMN') ;

insert into rate_plans values (5, 'RatePlan', 'N', 'RatePlan', 'GS', 'GSM', 'Zone2', 'AllDay', 'V', 'OF', 0,0, 60, 5, 0,0,0,0,0,0,0,0,0,0,0,0,0,3,'HPMN') ;
insert into rate_plans values (6, 'RatePlan', 'N', 'RatePlan', 'TF', 'GSM', 'Zone2', 'AllDay', 'V', 'OF', 0,0, 60, 6, 0,0,0,0,0,0,0,0,0,0,0,0,0,3,'HPMN') ;

insert into rate_plans values (7, 'RatePlan', 'N', 'RatePlan', 'GS', 'GSM', 'Zone1', 'AllDay', 'S', 'OF', 0,0, 0, 1, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,'HPMN') ;
insert into rate_plans values (8, 'RatePlan', 'N', 'RatePlan', 'TF', 'GSM', 'Zone1', 'AllDay', 'S', 'OF', 0,0, 0, 2, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,'HPMN') ;

insert into rate_plans values (9, 'RatePlan', 'N', 'RatePlan', 'GS', 'GSM', 'Zone2', 'AllDay', 'S', 'OF', 0,0, 0, 5, 0,0,0,0,0,0,0,0,0,0,0,0,0,3,'HPMN') ;
insert into rate_plans values (10, 'RatePlan', 'N', 'RatePlan', 'TF', 'GSM', 'Zone2', 'AllDay', 'S', 'OF', 0,0, 0, 6, 0,0,0,0,0,0,0,0,0,0,0,0,0,3,'HPMN') ;

insert into rate_plans values (11, 'RatePlan', 'N', 'RatePlan', 'GS', 'Default', 'Default', 'AllDay', 'G', 'OF', 0,0, 512, 1, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,'HPMN') ;
insert into rate_plans values (12, 'RatePlan', 'N', 'RatePlan', 'TF', 'Default', 'Default', 'AllDay', 'G', 'OF', 0,0, 512, 2, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,'HPMN') ;

insert into rate_plans values (13, 'RatePlan', 'N', 'RatePlan1', 'GS', 'Default', 'Default', 'AllDay', 'G', 'OF', 512,5, 1, 1, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,'HPMN') ;
insert into rate_plans values (14, 'RatePlan', 'N', 'RatePlan1', 'TF', 'Default', 'Default', 'AllDay', 'G', 'OF', 512,8, 1, 1, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,'HPMN') ;

insert into rate_plans values (15, 'RatePlan', 'N', 'RatePlan', 'GS', 'Default', 'Default', 'AllDay', 'G', 'OF', 0,0, 512, 5, 0,0,0,0,0,0,0,0,0,0,0,0,0,3,'HPMN') ;
insert into rate_plans values (16, 'RatePlan', 'N', 'RatePlan', 'TF', 'Default', 'Default', 'AllDay', 'G', 'OF', 0,0, 512, 6, 0,0,0,0,0,0,0,0,0,0,0,0,0,3,'HPMN') ;

insert into rate_plans values (17, 'RatePlan', 'N', 'RatePlan', 'GS', 'Default', 'Default', 'AllDay', 'M', 'OF', 0,0, 0, 3, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,'HPMN') ;
insert into rate_plans values (18, 'RatePlan', 'N', 'RatePlan', 'TF', 'Default', 'Default', 'AllDay', 'M', 'OF', 0,0, 0, 6, 0,0,0,0,0,0,0,0,0,0,0,0,0,1,'HPMN') ;

insert into rate_plans values (19, 'RatePlan', 'N', 'RatePlan', 'GS', 'Default', 'Default', 'AllDay', 'M', 'OF', 0,0, 0, 5, 0,0,0,0,0,0,0,0,0,0,0,0,0,3,'HPMN') ;
insert into rate_plans values (20, 'RatePlan', 'N', 'RatePlan', 'TF', 'Default', 'Default', 'AllDay', 'M', 'OF', 0,0, 0, 6, 0,0,0,0,0,0,0,0,0,0,0,0,0,3,'HPMN') ;
--insert into RATER_SPECIAL_NUMBERS(ID,DESCRIPTION,SPECIAL_NUMBER,SERVICE_CATEGORY,MIN_CHARGEABLE_UNIT,MIN_CHARGE,PULSE,RATE,PSTN_MIN_CHARGEABLE_UNIT,PSTN_MIN_CHARGE,PSTN_PULSE,PSTN_RATE,EXTRA_CHARGE,CALL_TYPE,MATCH_TYPE) values(1024, 'RSN1', '+40100', 'GS',10,12,'1', '35.23',12,13,1,2,12, 'S','','PM');
insert into RATER_SPECIAL_NUMBERS  (ID,DESCRIPTION,SPECIAL_NUMBER,SERVICE_CATEGORY,PULSE,RATE,CALL_TYPE,PMN,MATCH_TYPE) values (1024, 'RSN1', '+40100', 'GS', '1', '35.23', 'S','','PM') ;

insert into NETWORKS_RATER_SPECIAL_NUMBERS (RATER_SPECIAL_NUMBER_ID, NETWORK_ID) values(1024,1025) ;

insert into default_rates values  (1024, 'IV' , 'V', 60, 4) ;

insert into friends_family values ('+96723456124', '+9671234453435,+96771134567') ;

update configurations set value = '10' where config_key = 'OPERATOR_COMMISION_VALUE' ;
update configurations set value = '20' where config_key = 'DEFAULT_SDR_RATE' ;

insert into sdr_rates values (1, to_date('2009/10/01 00:00:00', 'yyyy/mm/dd hh24:mi:ss'), to_date('2009/10/30 23:59:59', 'yyyy/mm/dd hh24:mi:ss'), 10) ;
insert into NETWORKS_SDR_RATES select id, 1 from networks ;
commit;

EOF
