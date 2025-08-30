$SQL_COMMAND -s /nolog << EOF
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
whenever oserror exit 5 ;
set serveroutput on ;

delete from ai_discretization_bands where network_id in (1, 1024) and profile_type = 4 ;

---- For Network 1
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20000, 4, 'DURATION', 0, 180, 1, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20001, 4, 'DURATION', 180, 300, 1, 75) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20002, 4, 'DURATION', 300, 600, 1, 70) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20003, 4, 'DURATION', 600, 900, 1, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20004, 4, 'DURATION', 900, 1500, 1, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20005, 4, 'DURATION', 1500, 2100, 1, 40) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20006, 4, 'DURATION', 2100, 2700, 1, 35) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20007, 4, 'DURATION', 2700, 3300, 1, 25) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20008, 4, 'DURATION', 3300, 4000, 1, 20) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20009, 4, 'DURATION', 4000, 1000000000000, 1, 15) ;

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30000, 4, 'VALUE', 0, 5, 1, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30001, 4, 'VALUE', 5, 10, 1, 75) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30002, 4, 'VALUE', 10, 20, 1, 70) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30003, 4, 'VALUE', 20, 50, 1, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30004, 4, 'VALUE', 50, 100, 1, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30005, 4, 'VALUE', 100, 200, 1, 40) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30006, 4, 'VALUE', 200, 500, 1, 30) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30007, 4, 'VALUE', 500, 1000, 1, 20) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30008, 4, 'VALUE', 1000, 2000, 1, 20) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30009, 4, 'VALUE', 2000, 1000000000000, 1, 15) ;

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20000, 4, 'CALLCOUNT', 0, 180, 1, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20001, 4, 'CALLCOUNT', 180, 500, 1, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20002, 4, 'CALLCOUNT', 500, 1000, 1, 30) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20003, 4, 'CALLCOUNT', 1000, 1000000000000, 1, 20) ;

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 40000, 4, 'VOLUME', 0, 10, 1, 70) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 40001, 4, 'VOLUME', 10, 40, 1, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 40002, 4, 'VOLUME', 40, 100, 1, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 40003, 4, 'VOLUME', 100, 500, 1, 40) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 40004, 4, 'VOLUME', 500, 1000000000000, 1, 25) ;

---- For Network 1024

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20010, 4, 'DURATION', 0, 150, 1024, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20011, 4, 'DURATION', 150, 300, 1024, 70) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20012, 4, 'DURATION', 300, 500, 1024, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20013, 4, 'DURATION', 500, 1000, 1024, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20014, 4, 'DURATION', 1000, 1500, 1024, 40) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20015, 4, 'DURATION', 1500, 3000, 1024, 30) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20016, 4, 'DURATION', 3000, 4500, 1024, 20) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20017, 4, 'DURATION', 4500, 6000, 1024, 10) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20018, 4, 'DURATION', 6000, 1000000000000, 1024, 10) ;

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30010, 4, 'VALUE', 0, 1, 1024, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30011, 4, 'VALUE', 1, 10, 1024, 70) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30012, 4, 'VALUE', 10, 25, 1024, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30013, 4, 'VALUE', 25, 50, 1024, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30014, 4, 'VALUE', 50, 75, 1024, 40) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30015, 4, 'VALUE', 75, 150, 1024, 35) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30016, 4, 'VALUE', 150, 300, 1024, 30) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 30017, 4, 'VALUE', 300, 1000000000000, 1024, 20) ;

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20000, 4, 'CALLCOUNT', 0, 100, 1024, 80) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20001, 4, 'CALLCOUNT', 100, 600, 1024, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20002, 4, 'CALLCOUNT', 600, 2000, 1024, 30) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 20003, 4, 'CALLCOUNT', 2000, 1000000000000, 1024, 10) ;

insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 40005, 4, 'VOLUME', 0, 20, 1024, 70) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 40006, 4, 'VOLUME', 20, 100, 1024, 60) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 40007, 4, 'VOLUME', 100, 500, 1024, 50) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 40008, 4, 'VOLUME', 500, 1000, 1024, 30) ;
insert into ai_discretization_bands(band_id, profile_type, field_name, min_range_value , max_range_value, network_id, tolerance)
	values( 40009, 4, 'VOLUME', 1000, 1000000000000, 1024, 20) ;

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
