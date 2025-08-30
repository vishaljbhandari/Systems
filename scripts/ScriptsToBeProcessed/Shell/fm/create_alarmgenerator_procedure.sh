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

--CREATE OR REPLACE TYPE split_tbl AS TABLE OF varchar2(32767);
--/

CREATE OR REPLACE PACKAGE AlarmGenerator
AS
	PROCEDURE CreateManualAlarm(reference_type in number, reference_value in varchar2, agg_type in number, agg_value in varchar2, network in varchar2, score in number, rule_name in varchar2, alarm_comment in varchar2, subscriber_type in number, user_id in varchar2, owner_type in number DEFAULT 2, reference_value_category_id in number, ref_id in number, record_string in varchar2 DEFAULT NULL, record_category in number DEFAULT 0, min_day number DEFAULT 0, max_day number DEFAULT 0) ;
END AlarmGenerator ;
/

CREATE OR REPLACE PACKAGE BODY AlarmGenerator AS
	PROCEDURE CreateManualAlarm(reference_type in number, reference_value in varchar2, agg_type in number, agg_value in varchar2, network in varchar2, score in number, rule_name in varchar2,  alarm_comment in varchar2, subscriber_type in number, user_id in varchar2, owner_type in number DEFAULT 2, reference_value_category_id in number, ref_id in number, record_string in varchar2 DEFAULT NULL, record_category in number DEFAULT 0, min_day number DEFAULT 0, max_day number DEFAULT 0)
	AS
		alarm_gen_id 		number(20) := 10 ;
		reference_id 		number(20) := 4  ;
		ALARM_STATUS_NEW 	number(2) := 6 ;
	BEGIN
			INSERT INTO alarms (network_id, id, reference_id, alert_count, created_date, modified_date, status, user_id,
			owner_type, score, value, cdr_count, is_visible,rule_ids, reference_type, reference_value) VALUES (network,
			alarm_gen_id, reference_id, 1, sysdate, sysdate, ALARM_STATUS_NEW, user_id, owner_type, score, 0, 0, 1,' ', reference_type, reference_value);
	END CreateManualAlarm ;
END AlarmGenerator ;
/

CREATE OR REPLACE FUNCTION Split ( InputValue IN VARCHAR2, Position IN PLS_INTEGER, Delimiter IN VARCHAR2 DEFAULT ',') RETURN VARCHAR2
IS
  GrpVal VARCHAR2(20000) := Delimiter || InputValue ;
  Ret1      PLS_INTEGER ;
  Ret2      PLS_INTEGER ;
BEGIN
  Ret1 := INSTR( GrpVal, Delimiter, 1, Position ) ;
  IF Ret1 > 0 THEN
    Ret2 := INSTR( GrpVal, Delimiter, 1, Position + 1) ;
    IF Ret2 = 0 THEN Ret2 := LENGTH( GrpVal ) + 1 ; END IF ;
    RETURN( SUBSTR( GrpVal, Ret1+1, Ret2 - Ret1-1 ) ) ;
  ELSE
    RETURN NULL ;
  END IF ;
END Split;
/

CREATE OR REPLACE FUNCTION CompareCSV(Groups IN VARCHAR2, CompareGroups IN VARCHAR2)
RETURN PLS_INTEGER
IS
   Value      VARCHAR2(100) ;
   TempGroups VARCHAR2(4000) ;
   i          PLS_INTEGER := 1 ; 
   Result	  PLS_INTEGER := 0 ;
 BEGIN
   TempGroups := ',' || Groups || ',' ;
   LOOP
     Value := Split (CompareGroups, i , ',') ;
     EXIT WHEN Value IS NULL ;
	 Result := INSTR (TempGroups, ',' || Value || ',') ; 
	 IF Result > 0 THEN
		RETURN 1 ;
	 END IF ;
     i := i + 1 ;
   END LOOP ;
return 0 ;
END CompareCSV ;
/

EOF
