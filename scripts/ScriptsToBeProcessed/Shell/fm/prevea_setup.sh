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

DROP SEQUENCE PREVEA_USER_ROLES_SEQ ;
DELETE FROM PREVEA_USERS ;
DROP TABLE PREVEA_USERS ;
DROP SEQUENCE PREVEA_USERS_SEQ ;
DELETE FROM PREVEA_USER_ROLES ;
DROP TABLE PREVEA_USER_ROLES ;
DELETE FROM PREVEA_ROLES ;
DROP TABLE PREVEA_ROLES ;

CREATE SEQUENCE PREVEA_USER_ROLES_SEQ INCREMENT BY 1 NOMAXVALUE MINVALUE 4 NOCYCLE CACHE 20 ORDER   ;
CREATE SEQUENCE PREVEA_USERS_SEQ INCREMENT BY 1 NOMAXVALUE MINVALUE 4 NOCYCLE CACHE 20 ORDER   ;
CREATE TABLE PREVEA_USERS (
     	ID               NUMBER ,
       	USERNAME         VARCHAR2(100), 
       	PASSWORD         VARCHAR2(100),
       	DESCRIPTION      VARCHAR2(100),
       	FIRST_NAME       VARCHAR2(100),
       	MIDDLE_NAME      VARCHAR2(100),
       	LAST_NAME        VARCHAR2(100),
       	CONTACT_NUMBER   VARCHAR2(100),   
       	CONTACT_ADDRESS  VARCHAR2(100), 
     	EMAILID          VARCHAR2(100),
       	PASSWORD_EXPIRES DATE ,   
       	DAYS_TO_EXPIRE   NUMBER,
     	TEAM_ID NUMBER )   ;
 
CREATE TABLE PREVEA_ROLES (
     	ID               NUMBER ,
       	NAME             VARCHAR2(50) , 
       	DESCRIPTION      VARCHAR2(100) ) ;

CREATE TABLE PREVEA_USER_ROLES (
     	ID               NUMBER ,
       	USER_ID          NUMBER , 
       	ROLE_ID          NUMBER  ) ;

INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (1,'smith' ,'asdse' , sysdate+99 ,99) ;
INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (2,'james' ,'asdse' , sysdate+99 ,99) ;
INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (3,'anderson' ,'asdse' , sysdate+99 ,99) ;

INSERT INTO PREVEA_ROLES( ID , NAME , DESCRIPTION ) VALUES (1,'guest' ,'guest') ;
INSERT INTO PREVEA_ROLES( ID , NAME , DESCRIPTION ) VALUES (0,'admin','admin') ;

INSERT INTO PREVEA_USER_ROLES( ID , USER_ID ,ROLE_ID ) VALUES (1,1,1) ;
INSERT INTO PREVEA_USER_ROLES( ID , USER_ID ,ROLE_ID ) VALUES (2,2,2) ;
INSERT INTO PREVEA_USER_ROLES( ID , USER_ID ,ROLE_ID ) VALUES (3,3,3) ;

COMMIT ;
EOF

