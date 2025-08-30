$ORACLE_HOME/bin/sqlplus $DB_USER/$DB_PASSWORD << EOF

whenever oserror exit 1 ;
whenever sqlerror exit 1 ;

delete from voucher_validity_detail ;

insert into voucher_validity_detail (voucher_value, validity_period, grace_period, msisdn_validity_period) values (50, 3, 0, 180) ;
insert into voucher_validity_detail (voucher_value, validity_period, grace_period, msisdn_validity_period) values (300, 21, 0, 180) ;
insert into voucher_validity_detail (voucher_value, validity_period, grace_period, msisdn_validity_period) values (600, 45, 0, 180) ;
insert into voucher_validity_detail (voucher_value, validity_period, grace_period, msisdn_validity_period) values (-1, 3, 0, 180) ;

quit success commit ;
EOF
