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

INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (4,'Neil' ,'asdse' , sysdate+99 ,99) ;
INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (5,'sabrish' ,'asdse' , sysdate+99 ,99) ;
INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (6,'demo123' ,'asdse' , sysdate+99 ,99) ;
INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (7,'kiran' ,'asdse' , sysdate+99 ,99) ;
INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (8,'radmin' ,'asdse' , sysdate+99 ,99) ;

INSERT INTO PREVEA_USER_ROLES( ID , USER_ID ,ROLE_ID ) VALUES (4,4,3) ;
INSERT INTO PREVEA_USER_ROLES( ID , USER_ID ,ROLE_ID ) VALUES (5,5,3) ;
INSERT INTO PREVEA_USER_ROLES( ID , USER_ID ,ROLE_ID ) VALUES (6,6,3) ;
INSERT INTO PREVEA_USER_ROLES( ID , USER_ID ,ROLE_ID ) VALUES (7,7,3) ;
INSERT INTO PREVEA_USER_ROLES( ID , USER_ID ,ROLE_ID ) VALUES (8,8,3) ;

COMMIT ;
EOF

