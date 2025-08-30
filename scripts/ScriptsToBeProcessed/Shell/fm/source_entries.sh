#/*******************************************************************************
# *	Copyright (c) Subex Azure Limited 2006. All rights reserved.	           *
# *	The copyright to the computer program(s) herein is the property of Subex   *
# *	Azure Limited. The program(s) may be used and/or copied with the written *
# *	permission from Subex Azure Limited or in accordance with the terms and  *
# *	conditions stipulated in the agreement/contract under which the program(s) *
# *	have been supplied.                                                        *
# *******************************************************************************/

#!/bin/bash

 $1 << EOF
SET FEEDBACK OFF;

insert into source (id, description) values (1,'Alarm Management');
insert into source (id, description) values (2,'Autothreshold Configuration');
insert into source (id, description) values (3,'Case Management');
insert into source (id, description) values (4,'CMM Management');
insert into source (id, description) values (5,'Event Management');
insert into source (id, description) values (6,'Exposure Management');
insert into source (id, description) values (7,'Hot List Management');
insert into source (id, description) values (8,'Nickname Management');
insert into source (id, description) values (9,'Server Configuration');
insert into source (id, description) values (10,'Subscriber Management');
insert into source (id, description) values (11,'User Authentication');
insert into source (id, description) values (12,'User Management');
insert into source (id, description) values (13,'Datasource');
insert into source (id, description) values (14,'CDR View');
insert into source (id, description) values (15,'Smart Pattern');
insert into source (id, description) values (16,'Recharge Log View');
insert into source (id, description) values (18,'System Alert');
insert into source (id, description) values (19,'Groups');
insert into source (id, description) values (20,'Grouping Rule');
insert into source (id, description) values (21,'Rule Configuration');
insert into source (id, description) values (22,'Record View');
insert into source (id, description) values (23,'Smart Pattern Rule Configuration');
insert into source (id, description) values (24,'Named Filter Management');
insert into source (id, description) values (25,'Auto Suspect Numbering');
insert into source (id, description) values (26,'Rating Configuration');
insert into source (id, description) values (27,'Alarm Adminstration');

commit ;
EOF
