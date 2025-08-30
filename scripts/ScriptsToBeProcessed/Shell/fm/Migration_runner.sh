#!/usr/bin/env bash
./Migration_after_7.3.2_for_alarm_attachment.sh
sqlplus $DB_USER/$DB_PASSWORD @Migration_after_7.3.2.sql

