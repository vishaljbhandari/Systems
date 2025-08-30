#/*******************************************************************************
# *	Copyright (c) Subex Azure Limited 2006. All rights reserved.	           *
# *	The copyright to the computer program(s) herein is the property of Subex   *
# *	Azure Limited. The program(s) may be used and/or copied with the written *
# *	permission from Subex Azure Limited or in accordance with the terms and  *
# *	conditions stipulated in the agreement/contract under which the program(s) *
# *	have been supplied.                                                        *
# *******************************************************************************/

#!/bin/bash

echo $1
  $1 << EOF
  SET FEEDBACK OFF;

  DROP SEQUENCE PREVEA_SESSIONS_SEQ  ;
  DROP TABLE  PREVEA_SESSIONS ;
  DROP TABLE PREVEA_USERS ;
  DROP TABLE user_tbl ;

  CREATE SEQUENCE PREVEA_SESSIONS_SEQ INCREMENT BY 1 NOMAXVALUE MINVALUE 1   ;
  CREATE TABLE PREVEA_SESSIONS (ID NUMBER , USER_ID  NUMBER ,  EXPIRES_ON DATE , IPADDRESS VARCHAR2(20))   ;
 
  INSERT INTO PREVEA_SESSIONS VALUES  (1, 1,  SYSDATE+10, '10.113.52.178') ;
  INSERT INTO PREVEA_SESSIONS VALUES  (2, 2,  SYSDATE+10, '10.113.52.178') ;
  INSERT INTO PREVEA_SESSIONS VALUES  (3, 3,  SYSDATE+10, '10.113.52.178') ;
 
  DELETE FROM ROLE_OPTIONS WHERE OPTION_ID NOT LIKE '%dac_FILTER%' ;
  DELETE FROM RULES WHERE USER_ID = 'ANDERSON' AND IS_ACTIVE=1 AND CATEGORY ='coloring_rules' ;

  CREATE TABLE prevea_users (
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
  TEAM_ID          NUMBER ) ; 

  CREATE TABLE USER_TBL (
  USR_ID             NUMBER(10) NOT NULL,
  PIG_ID             NUMBER(10) NOT NULL,
  USR_NAME           NVARCHAR2(255) NOT NULL,
  USR_PASSWORD       NVARCHAR2(255) NOT NULL,
  USR_FORENAME       NVARCHAR2(255) NOT NULL,
  USR_SURNAME        NVARCHAR2(255) NOT NULL,
  USR_EMAIL_ADDRESS  NVARCHAR2(255) NOT NULL,
  USR_DISABLED_FL    CHAR(1) NOT NULL,
  USR_DELETE_FL      CHAR(1) NOT NULL,
  USR_VERSION_ID     NUMBER(10) NOT NULL,
  PTN_ID             NUMBER(10) NOT NULL) ;

  INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (1,'Neil' ,'asdse' , sysdate+99 ,99) ;
  INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (2,'radmin' ,'asdse' , sysdate+99 ,99) ;
  INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (3,'sabrish' ,'asdse' , sysdate+99 ,99) ;
  INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (4,'demo123' ,'asdse' , sysdate+99 ,99) ;
  INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (5,'irfann' ,'asdse' , sysdate+99 ,99) ;
  INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (6,'aloks' ,'asdse' , sysdate+99 ,99) ;
  INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (7,'aloks1' ,'asdse' , sysdate+99 ,99) ;
  INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (8,'irfann1' ,'asdse' , sysdate+99 ,99) ;
  INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (9,'anderson' ,'asdse' , sysdate+99 ,99) ;
  INSERT INTO PREVEA_USERS( ID , USERNAME,PASSWORD , PASSWORD_EXPIRES, DAYS_TO_EXPIRE) VALUES (10,'kiran' ,'asdse' , sysdate+99 ,99) ;

commit ;
EOF
