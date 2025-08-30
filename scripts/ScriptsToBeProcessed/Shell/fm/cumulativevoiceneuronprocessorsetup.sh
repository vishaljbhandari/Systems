$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
set serveroutput on ;

delete from ai_discretization_bands where network_id in (1, 1024) and profile_type = 2 ;

---- For Network 1
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20000, 2, 'DURATION', 0, 180, 1, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20001, 2, 'DURATION', 180, 300, 1, 75) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20002, 2, 'DURATION', 300, 600, 1, 70) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20003, 2, 'DURATION', 600, 900, 1, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20004, 2, 'DURATION', 900, 1500, 1, 55) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20005, 2, 'DURATION', 1500, 2100, 1, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20006, 2, 'DURATION', 2100, 2700, 1, 45) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20007, 2, 'DURATION', 2700, 3300, 1, 40) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20008, 2, 'DURATION', 3300, 4000, 1, 30) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20009, 2, 'DURATION', 4000, 1000000000000, 1, 20) ;

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30000, 2, 'VALUE', 0, 5, 1, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30001, 2, 'VALUE', 5, 10, 1, 75) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30002, 2, 'VALUE', 10, 20, 1, 70) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30003, 2, 'VALUE', 20, 50, 1, 65) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30004, 2, 'VALUE', 50, 100, 1, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30005, 2, 'VALUE', 100, 200, 1, 55) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30006, 2, 'VALUE', 200, 500, 1, 40) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30007, 2, 'VALUE', 500, 1000, 1, 30) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30008, 2, 'VALUE', 1000, 2000, 1, 20) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30009, 2, 'VALUE', 2000, 1000000000000, 1, 20) ;

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20000, 2, 'CALLCOUNT', 0, 180, 1, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20001, 2, 'CALLCOUNT', 180, 500, 1, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20002, 2, 'CALLCOUNT', 500, 1000, 1, 30) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20003, 2, 'CALLCOUNT', 1000, 1000000000000, 1, 10) ;

---- For Network 1024

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20010, 2, 'DURATION', 0, 150, 1024, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20011, 2, 'DURATION', 150, 300, 1024, 75) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20012, 2, 'DURATION', 300, 500, 1024, 70) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20013, 2, 'DURATION', 500, 1000, 1024, 65) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20014, 2, 'DURATION', 1000, 1500, 1024, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20015, 2, 'DURATION', 1500, 3000, 1024, 55) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20016, 2, 'DURATION', 3000, 4500, 1024, 45) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20017, 2, 'DURATION', 4500, 6000, 1024, 25) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20018, 2, 'DURATION', 6000, 1000000000000, 1024, 20) ;

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30010, 2, 'VALUE', 0, 1, 1024, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30011, 2, 'VALUE', 1, 10, 1024, 70) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30012, 2, 'VALUE', 10, 25, 1024, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30013, 2, 'VALUE', 25, 50, 1024, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30014, 2, 'VALUE', 50, 75, 1024, 40) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30015, 2, 'VALUE', 75, 150, 1024, 30) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30016, 2, 'VALUE', 150, 300, 1024, 20) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30017, 2, 'VALUE', 300, 1000000000000, 1024, 10) ;

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20000, 2, 'CALLCOUNT', 0, 100, 1024, 90) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20001, 2, 'CALLCOUNT', 100, 600, 1024, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20002, 2, 'CALLCOUNT', 600, 2000, 1024, 20) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20003, 2, 'CALLCOUNT', 2000, 1000000000000, 1024, 10) ;

commit ;
quit ;
EOF

if test $? -eq 5 
then
    exitval=1
else
    exitval=0
fi

exit $exitval
