#!/bin/bash
#/*******************************************************************************
# * Copyright (c) Subex Systems Limited 2005. All rights reserved.              *
# * The copyright to the computer program(s) herein is the property of Subex    *
# * Systems Limited. The program(s) may be used and/or copied with the written  *
# * permission from Subex Systems Limited or in accordance with the terms and   *
# * conditions stipulated in the agreement/contract under which the program(s)  *
# * have been supplied.                                                         *
# *******************************************************************************/

. rangerenv.sh

on_exit ()
{
    rm -f $RANGERHOME/RangerData/Scheduler/SimBoxFraudSummaryNetworkPID$1
}
trap on_exit EXIT

sqlplus -s /nolog << EOF
CONNECT_TO_SQL

declare
	DayofYear number(5);
	retention_period number(5) ;

begin

		select to_char(sysdate,'ddd') into DayofYear from dual;

		insert into du_voice_sim_box_summary select distinct phone_number, subscriber_id, sum(decode(record_type,1,1,0)), sum(decode(record_type,2,1,0)), sum(decode(record_type,3,1,0)), count(distinct geographic_position), to_date(DayofYear, 'DDD') from cdr where day_of_year=DayofYear and service_type=1 group by phone_number, subscriber_id ;

		insert into du_sms_sim_box_summary select distinct phone_number, subscriber_id, sum(decode(record_type,1,1,0)), sum(decode(record_type,2,1,0)), sum(decode(record_type,3,1,0)), count(distinct geographic_position), to_date(DayofYear, 'DDD') from cdr where day_of_year=DayofYear and service_type=2 group by phone_number, subscriber_id ;

	select to_number(value) into retention_period from configurations where config_key='SIM_BOX_SUMMARY_RETENTION_PERIOD' ;	

	delete from du_voice_sim_box_summary where trunc(traffic_date) <= trunc(to_date(DayofYear-retention_period,'DDD')) ;
	delete from du_sms_sim_box_summary where trunc(traffic_date) <= trunc(to_date(DayofYear-retention_period,'DDD')) ;

end;
/
EOF

exit 0
