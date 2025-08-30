sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from ds_configuration  where id like '%GBXUS_RATING.%';

insert into ds_configuration (id, value) values ('GBXUS_RATING.LONGDISTANCE', '10.0');
insert into ds_configuration (id, value) values ('GBXUS_RATING.INTERNATIONAL', '4.20');
insert into ds_configuration (id, value) values ('GBXUS_RATING.INCOMING', '0');
insert into ds_configuration (id, value) values ('GBXUS_RATING.LONG_DIST_PULSE_DURATION', '60');
insert into ds_configuration (id, value) values ('GBXUS_RATING.INTERNATIONAL_PULSE_DURATION', '60');
insert into ds_configuration (id, value) values ('GBXUS_RATING.INCOMING_PULSE_DURATION', '60');

commit;
quit;
EOF
if test $? -eq 5 
then
    exitval=1
else
    exitval=0
fi

exit $exitval

