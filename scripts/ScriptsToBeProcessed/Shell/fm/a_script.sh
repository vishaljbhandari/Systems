#!/bin/bash

VAR_1=`date "+%d/%m/%Y %T"`
VAR_2="" 
VAR_2="8"
VAR_2="0"

if [ "$1" == "999" ]  
then
	shift
fi ;

on_exit ()
{
	if [ "$4" == "" ]
	then
		
		
		
		
	fi ;	
}
trap on_exit EXIT 

. IF_SOME_SCRIPT.sh

echo "$*" >/tmp/generate.sh

FUNCTION_01()
{
    day=$1
    month=$2
    year=$3

    # if it is the first day of the month
    if [ $day -eq 01 ]
    then
        # if it is the first month of the year
        if [ $month -eq 01 ]
        then
            # make the month as 12
            month=12

            # deduct the year by one
            year=`expr $year - 1`
        else
            # deduct the month by one
            month=`expr $month - 1`
        fi

	day=`cal $month $year | awk 'NF != 0{ last = $0 }; END{ print last }' |  awk '{ print $NF }'`
    else
        day=`expr $day - 1`
    fi

    StartDate=$day/$month/$year
	EndDate=$StartDate
}

GetConfigEntry ()
{
	TempConfigValue=`sqlplus -s $DB_USER/$DB_PASSWORD << EOF
		set heading off ;
		set feedback off ;
		select value from configurations where config_key = '$1' ;
EOF`
	
	if [ "x""$TempConfigValue" == "x" ]; then
		return 1
	fi
	return 0
}

Initialize ()
{
sqlplus -s $DB_USER/$DB_PASSWORD << EOF > $NIKIRAROOT/LOG/GenerateHURInStandardFormat$StartDate.log
	SET FEEDBACK OFF ;
	DROP TABLE TMP_REP_DATA ;
	DROP TABLE TMP_REP_SEC_C_DATA ;
	DROP TABLE TMP_REP_GPRS_DATA ;
	DROP TABLE TMP_REP_SEC_C_GPRS_DATA ;
	CREATE TABLE TMP_REP_DATA
	(
	 	RECORD_TYPE				NUMBER,
		IMSI					VARCHAR2(40),
		DATE_FIRST_EVENT		DATE,
		TIME_FIRST_EVENT		DATE,
		DATE_LAST_EVENT			DATE,
		TIME_LAST_EVENT			DATE,
		DC						NUMBER,
		NC						NUMBER,
		VOLUME					NUMBER,
		SDR						NUMBER
	) ;

	CREATE TABLE TMP_REP_GPRS_DATA
	(
	 	RECORD_TYPE				NUMBER,
		IMSI					VARCHAR2(40),
		DATE_FIRST_EVENT		DATE,
		TIME_FIRST_EVENT		DATE,
		DATE_LAST_EVENT			DATE,
		TIME_LAST_EVENT			DATE,
		DC						NUMBER,
		NC						NUMBER,
		VOLUME					NUMBER,
		SDR						NUMBER
	) ;

	CREATE TABLE TMP_REP_SEC_C_DATA
	(
	 	RECORD_TYPE             NUMBER,
	 	IMSI					VARCHAR2(40),
		DATE_FIRST_EVENT		DATE,
		DATE_LAST_EVENT			DATE,
		COUNTRY_CODE			VARCHAR2(20),
		DC						NUMBER,
		NC						NUMBER,
		VOLUME					NUMBER,
		SDR						NUMBER
	) ;

	CREATE TABLE TMP_REP_SEC_C_GPRS_DATA
	(
	 	RECORD_TYPE             NUMBER,
	 	IMSI					VARCHAR2(40),
		DATE_FIRST_EVENT		DATE,
		DATE_LAST_EVENT			DATE,
		COUNTRY_CODE			VARCHAR2(20),
		DC						NUMBER,
		NC						NUMBER,
		VOLUME					NUMBER,
		SDR						NUMBER
	) ;
EOF
}

CleanUp ()
{
sqlplus -s $DB_USER/$DB_PASSWORD << EOF >> $NIKIRAROOT/LOG/GenerateHURInStandardFormat$StartDate.log
set serveroutput on ;
set feedback on ;
	DROP TABLE TMP_REP_DATA ;
	DROP TABLE TMP_REP_SEC_C_DATA ;
	DROP TABLE TMP_REP_GPRS_DATA ;
	DROP TABLE TMP_REP_SEC_C_GPRS_DATA ;
DECLARE
	START_DATE DATE  ;
	END_DATE   DATE  ;
	CLEAN_UP_DURATION NUMBER(10) := $CleanupDuration;
BEGIN
	START_DATE := to_date('$StartDate','dd/mm/yyyy HH24:mi:ss') ;
	END_DATE := to_date('$EndDate','dd/mm/yyyy HH24:mi:ss') ;
	if START_DATE < END_DATE
	then
		DELETE FROM INROAMER_HUR WHERE TIME_STAMP < (SYSDATE) - CLEAN_UP_DURATION ;
		DELETE FROM GPRS_INROAMER_HUR WHERE TIME_STAMP < (SYSDATE) - CLEAN_UP_DURATION ;
	end if ;	
	commit;
END ;
/
EOF
}

GenerateReport ()
{
#filePath=$1
filePath="AUTO_FILE_PATH"
id=0
if [ $# -eq 2 ]
then
filePath="MANL_FILE_PATH"
id=$2
fi
if [ "$Whom" != "ALL" ]
then
TAPFILE_NAME=`sqlplus -s $DB_USER/$DB_PASSWORD <<EOF

	SET SERVEROUTPUT ON ;
	SET FEEDBACK OFF ;
	SET LINE 300;
	SET TRIMSPOOL ON;
	DECLARE
		SEQ_NUMBER				VARCHAR2(10)  := NULL ;
		inboundseqnumber		NUMBER(20) ;
		EMAILID					VARCHAR2(256) ;
		FILENAME				VARCHAR2(128) ;
		dirpath					varchar2(128) ;
		TAPCODE					VARCHAR2(20) ;
		FUNCTION RESET_SEQUANCE (CURRENT_SEQUANCE VARCHAR2) RETURN NUMBER IS
		BEGIN
			IF SUBSTR(CURRENT_SEQUANCE, 5) = TO_CHAR(SYSDATE, 'YYYY') THEN
				RETURN 0 ;
			END IF;
			RETURN 1 ;
		END ;
	begin
		select INBOUND_SEQUENCE_NUMBER, replace(EMAIL_ID, '@', '^') into inboundseqnumber, EMAILID from ROAMER_PARTNER where tap_code='$Whom'  and rownum <2;
		select DIRECTORY_PATH into dirpath from all_directories where DIRECTORY_NAME='$filePath' and rownum <2;
		SEQ_NUMBER := LPAD(NVL(inboundseqnumber, '0000' || TO_CHAR(SYSDATE, 'YYYY')) + 10000, 8, 0) ;
	    	IF RESET_SEQUANCE (SEQ_NUMBER) = 1 THEN
		    SEQ_NUMBER := LPAD ('0001' || TO_CHAR(SYSDATE, 'YYYY'), 8, 0) ;
	    	END IF ;
	    	UPDATE ROAMER_PARTNER SET INBOUND_SEQUENCE_NUMBER = SEQ_NUMBER WHERE TAP_CODE = '$Whom' ;
		FILENAME:=dirpath||'/HUR_ALLZAFCC' || '$Whom' || SEQ_NUMBER || '.csv' || '~' || EMAILID ;
		dbms_output.put_line(FILENAME);
	end;
/

EOF`
else
	TAPFILE_NAME="$RANGERHOME/tmp/ALL_TAP_CODES.txt"
fi
echo $TAPFILE_NAME
FILENAME=`basename $TAPFILE_NAME |sed 's/\^/@/g'`
 echo $TAPFILE_NAME
sqlplus -s $DB_USER/$DB_PASSWORD << EOF >> $NIKIRAROOT/LOG/GenerateHURInStandardFormat$StartDate.log
	SET SERVEROUTPUT ON ;
	SET FEEDBACK ON ;
	SET LINE 30000 ;

	DECLARE
		REC_COUNT				NUMBER (10,0) := 0 ;
		TOTAL_VALUE				NUMBER (10,2) := 0 ;
		OUT_FILE 				UTL_FILE.FILE_TYPE ;
		LOG_FILE		        UTL_FILE.FILE_TYPE ;
		START_DATE				DATE := NULL ;
		END_DATE				DATE := NULL ;
		SEQ_NUMBER				VARCHAR2(10)  := NULL ;
		F_HHH					NUMBER(3) := 0 ;
		F_MM					NUMBER(2) := 0 ;
		F_SS					NUMBER(2) := 0 ;
		Temp					NUMBER(10) := 0 ;
		Temp1					NUMBER(10) := 0 ;
		Ret						VARCHAR2(7) := NULL ;
		LINE_COUNT				NUMBER(8) := 0 ;
		GIVEN_TAPCODES          VARCHAR2(100) :='$Whom' ;
		inboundseqnumber		NUMBER(20) ;
		inboundTHRESHOLD		NUMBER(16,6) ;
		EMAILID					VARCHAR2(256) ;
		smstreaty				NUMBER(2) ;
		hurenabled				NUMBER(2) ;
		FILENAME				VARCHAR2(528) ;
		dirpath					varchar2(128) ;
		TAPCODES                VARCHAR2(1000);
		APPENDSTRING            VARCHAR2(1000);
		RESULT_TAPCODE          VARCHAR2(100);
		TAP_COUNT               NUMBER(2) :=0;
		NO_OF_TAP_CODES         NUMBER(10) ;
		LOCATION                NUMBER(10) := 0 ;
		TOTAL_TAPCODE_COUNT     NUMBER(10) := 0;
		TAPCODE					VARCHAR2(20) ;
		LOG_END_TIME			VARCHAR2(25) ;
		REJECTED_TAPCODES       VARCHAR2(200) := '' ;
		REJECTED_TAPCODE_COUNT  NUMBER(10) := 0 ;
		TAP_CODES_PROCESSED		NUMBER(10) := 0 ;
		ACCEPTED_TAPCODES_COUNT NUMBER(10) := 0 ;
		ACCEPTED_TAPCODES       VARCHAR2(200) := '' ;
		INVALID_MONTH                   exception;
		rownumt				number:= 0 ;
        INVALID_DAY_FOR_GIVEN_MONTH     exception;
        INVALID_DAY_FOR_MONTH           exception;
        INVALID_YEAR                    exception;
		PRAGMA EXCEPTION_INIT(INVALID_MONTH, -01843);
        PRAGMA EXCEPTION_INIT(INVALID_DAY_FOR_GIVEN_MONTH, -01839);
        PRAGMA EXCEPTION_INIT(INVALID_DAY_FOR_MONTH, -01847);
        PRAGMA EXCEPTION_INIT(INVALID_YEAR, -01841);




		CURSOR ROAMER_PARTNER_LIST IS SELECT * FROM ROAMER_PARTNER ;
		
		CURSOR ROAMER_PARTNER_CDR IS SELECT * FROM TMP_REP_DATA  ;

		CURSOR ROAMER_PARTNER_GPRS_CDR IS SELECT * FROM TMP_REP_GPRS_DATA  ;

		CURSOR ROAMER_PARTNER_CDRS IS SELECT * FROM TMP_REP_DATA WHERE RECORD_TYPE=1 ;

		CURSOR ROAMER_PARTNER_GPRS_CDRS IS SELECT * FROM TMP_REP_GPRS_DATA WHERE RECORD_TYPE=1 ;

		CURSOR ROAMER_PARTNER_MMS_CDRS IS SELECT * FROM TMP_REP_GPRS_DATA WHERE RECORD_TYPE=2 ;

		CURSOR ROAMER_PARTNER_SMSGPRS_CDRS IS SELECT * FROM TMP_REP_GPRS_DATA WHERE RECORD_TYPE=3 ;
		
		CURSOR ROAMER_PARTNER_SMSCDRS IS SELECT * FROM TMP_REP_DATA WHERE RECORD_TYPE=2 ;

		CURSOR ROAMER_PARTNER_SUMMARY IS SELECT * FROM TMP_REP_DATA ;

		CURSOR ROAMER_PARTNER_SUB_CDRS IS SELECT * FROM TMP_REP_SEC_C_DATA WHERE RECORD_TYPE=1 ;

		CURSOR ROAMER_PARTNER_SMS_CDRS IS SELECT * FROM TMP_REP_SEC_C_DATA WHERE RECORD_TYPE=2 ;

--		CURSOR ROAMER_PARTNER_GPRSSMS_CDRS IS SELECT * FROM TMP_REP_SEC_C_GPRS_DATA WHERE RECORD_TYPE=3 ;

		
		FUNCTION VALIDATE_TAPCODES (SEARCHSTRING IN VARCHAR2, MAINSTRING VARCHAR2) RETURN NUMBER IS
    	BEGIN
        	LOCATION := INSTR(MAINSTRING,SEARCHSTRING);
        LOOP
            TOTAL_TAPCODE_COUNT := TOTAL_TAPCODE_COUNT+1 ;
            LOCATION := INSTR(MAINSTRING,SEARCHSTRING,LOCATION+1) ;
            IF LOCATION = 0 THEN
            EXIT ;
            END IF ;
        END LOOP ;
        
		RETURN TOTAL_TAPCODE_COUNT ;
    	END ;


		PROCEDURE PRINT_LINE (LINE IN VARCHAR2) IS
		BEGIN
			--UTL_FILE.PUT_LINE (OUT_FILE, LINE) ;
			insert into hur_output_store values(rownumt, '$filePath', LINE) ;
			LINE_COUNT := LINE_COUNT + 1 ;
			rownumt := rownumt + 1 ;
		END ;

		FUNCTION RESET_SEQUANCE (CURRENT_SEQUANCE VARCHAR2) RETURN NUMBER IS
		BEGIN
			IF SUBSTR(CURRENT_SEQUANCE, 5) = TO_CHAR(SYSDATE, 'YYYY') THEN
				RETURN 0 ;
			END IF;
			RETURN 1 ;
		END ;

		FUNCTION VALIDATE_DATE (TimeStamp IN VARCHAR2, ValidDate OUT DATE) RETURN NUMBER IS
		BEGIN
			SELECT TO_DATE(NVL(TimeStamp, '---'), 'DD/MM/YYYY HH24:MI:SS') INTO ValidDate FROM DUAL ;
			RETURN 0 ;
		EXCEPTION
					WHEN INVALID_MONTH THEN
                       	 dbms_output.put_line('InValid Month '||TimeStamp) ; 
						ValidDate := NULL ;
					RETURN 1 ;
                    WHEN INVALID_DAY_FOR_GIVEN_MONTH THEN
                       	 dbms_output.put_line('InValid Day For Given Month '||TimeStamp) ; 
						ValidDate := NULL ;
					RETURN 1 ;
                    WHEN INVALID_DAY_FOR_MONTH THEN
                       	 dbms_output.put_line('InValid Day For  Month '||TimeStamp) ; 
						ValidDate := NULL ;
					RETURN 1 ;
                    WHEN INVALID_YEAR THEN
                       	 dbms_output.put_line('InValid Year '||TimeStamp) ; 
						ValidDate := NULL ;
					RETURN 1 ;
					WHEN OTHERS THEN
						ValidDate := NULL ;
					RETURN 1 ;
		END ;

		FUNCTION DC_HHHMMSS (DURATION NUMBER) RETURN VARCHAR2 IS
		BEGIN
			Temp 	:= DURATION ;
			F_HHH 	:= FLOOR(Temp / 3600) ;
			Temp 	:= Temp - (F_HHH * 3600) ;
			F_MM  	:= FLOOR(Temp / 60) ;
			Temp 	:= Temp - (F_MM * 60) ;
			F_SS  	:= Temp ; 
			Ret 	:= LPAD (F_HHH, 3, 0) || LPAD (F_MM, 2, 0) || LPAD (F_SS, 2, 0) ;
			RETURN Ret ;
		END ;

		PROCEDURE PRINT_RECORDS(TAPCODE IN VARCHAR2 ) IS
		BEGIN
			REC_COUNT := 0 ;
				FOR ROAMER_PARTNER_REC IN ROAMER_PARTNER_CDR LOOP
				INSERT INTO TMP_REP_SEC_C_DATA SELECT SERVICE, IMSI,
					MIN(TIME_STAMP), MAX(TIME_STAMP), COUNTRY_CODE,
					SUM(DURATION), COUNT(*), NULL, SUM(VALUE)
				FROM INROAMER_HUR 
					WHERE IMSI = ROAMER_PARTNER_REC.IMSI AND
					SERVICE = ROAMER_PARTNER_REC.RECORD_TYPE AND
					TIME_STAMP BETWEEN ROAMER_PARTNER_REC.DATE_FIRST_EVENT AND ROAMER_PARTNER_REC.DATE_LAST_EVENT
					GROUP BY SERVICE, COUNTRY_CODE, IMSI ;
				END LOOP ;
				
				FOR ROAMER_PARTNER_REC IN ROAMER_PARTNER_GPRS_CDR LOOP
				INSERT INTO TMP_REP_SEC_C_GPRS_DATA SELECT GPRS_SERVICE, IMSI,
					MIN(TIME_STAMP), MAX(TIME_STAMP), COUNTRY_CODE,
					SUM(DURATION), COUNT(*),SUM(VOLUME) , SUM(VALUE)
				FROM GPRS_INROAMER_HUR 
					WHERE IMSI = ROAMER_PARTNER_REC.IMSI AND
					GPRS_SERVICE = ROAMER_PARTNER_REC.RECORD_TYPE AND
					TIME_STAMP BETWEEN ROAMER_PARTNER_REC.DATE_FIRST_EVENT AND ROAMER_PARTNER_REC.DATE_LAST_EVENT
					GROUP BY GPRS_SERVICE, COUNTRY_CODE, IMSI ;
				END LOOP ;

	-----------------------------------################# SET B OF HUR  ###################-------------------------------------------		
			
			
			FOR ROAMER_PARTNER_REC IN ROAMER_PARTNER_CDRS LOOP
				REC_COUNT := REC_COUNT + 1 ;
				PRINT_LINE ('C,' ||
						ROAMER_PARTNER_REC.IMSI || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.DATE_FIRST_EVENT, 'YYYYMMDD') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.TIME_FIRST_EVENT, 'HH24MISS') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.DATE_LAST_EVENT, 'YYYYMMDD') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.TIME_LAST_EVENT, 'HH24MISS') || ',' || 
						DC_HHHMMSS (ROAMER_PARTNER_REC.DC) || ',' ||
						ROAMER_PARTNER_REC.NC || ',,' || ROAMER_PARTNER_REC.SDR) ;
			END LOOP ;	

			FOR ROAMER_PARTNER_REC IN ROAMER_PARTNER_GPRS_CDRS LOOP
				REC_COUNT := REC_COUNT + 1 ;
				PRINT_LINE ('P,' ||
						ROAMER_PARTNER_REC.IMSI || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.DATE_FIRST_EVENT, 'YYYYMMDD') || ',,' ||
						TO_CHAR (ROAMER_PARTNER_REC.DATE_LAST_EVENT, 'YYYYMMDD') || ',,' ||
						DC_HHHMMSS (ROAMER_PARTNER_REC.DC) || ',' ||
						ROAMER_PARTNER_REC.NC ||','|| ROAMER_PARTNER_REC.VOLUME ||','|| ROAMER_PARTNER_REC.SDR) ;
			END LOOP ;	

			FOR ROAMER_PARTNER_REC IN ROAMER_PARTNER_MMS_CDRS LOOP
				REC_COUNT := REC_COUNT + 1 ;
				PRINT_LINE ('M,' ||
						ROAMER_PARTNER_REC.IMSI || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.DATE_FIRST_EVENT, 'YYYYMMDD') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.TIME_FIRST_EVENT, 'HH24MISS') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.DATE_LAST_EVENT, 'YYYYMMDD') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.TIME_LAST_EVENT, 'HH24MISS') || ',,' || 
						ROAMER_PARTNER_REC.NC || ',,' || ROAMER_PARTNER_REC.SDR) ;
			END LOOP ;	
           
   			     
			FOR ROAMER_PARTNER_REC IN ROAMER_PARTNER_SMSGPRS_CDRS LOOP
				REC_COUNT := REC_COUNT + 1 ;
				BEGIN
				select sms_treaty into smstreaty from roamer_partner where tap_code=TAPCODE;
					IF smstreaty=1
					THEN
						PRINT_LINE ('P,' ||
						ROAMER_PARTNER_REC.IMSI || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.DATE_FIRST_EVENT, 'YYYYMMDD') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.TIME_FIRST_EVENT, 'HH24MISS') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.DATE_LAST_EVENT, 'YYYYMMDD') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.TIME_LAST_EVENT, 'HH24MISS') || ',,' || 
						ROAMER_PARTNER_REC.NC ||',,'|| ROAMER_PARTNER_REC.SDR) ;
					END IF ;		
					EXCEPTION
						WHEN NO_DATA_FOUND THEN
						RETURN  ;
				END;
			END LOOP ;	
		  


			FOR ROAMER_PARTNER_REC IN ROAMER_PARTNER_SMSCDRS 
			LOOP
				REC_COUNT := REC_COUNT + 1 ;
				BEGIN
				select sms_treaty into smstreaty from roamer_partner where tap_code=TAPCODE;
					IF smstreaty=1
					THEN
						PRINT_LINE ('S,' ||
						ROAMER_PARTNER_REC.IMSI || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.DATE_FIRST_EVENT, 'YYYYMMDD') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.TIME_FIRST_EVENT, 'HH24MISS') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.DATE_LAST_EVENT, 'YYYYMMDD') || ',' ||
						TO_CHAR (ROAMER_PARTNER_REC.TIME_LAST_EVENT, 'HH24MISS') || ',,' || 
						   ROAMER_PARTNER_REC.NC || ',,' || ROAMER_PARTNER_REC.SDR) ;
					END IF ;		
					EXCEPTION
						WHEN NO_DATA_FOUND THEN
						RETURN  ;
				END;
			END LOOP ;
		END;

	PROCEDURE SPLIT_TAPCODES(LINE IN VARCHAR2)
	IS
	BEGIN
		TAPCODES := LINE||',' ;
		APPENDSTRING := TAPCODES;
		NO_OF_TAP_CODES := VALIDATE_TAPCODES(',',TAPCODES);
		FOR I IN 1..NO_OF_TAP_CODES
		LOOP
			RESULT_TAPCODE:=SUBSTR(APPENDSTRING,1,(INSTR(APPENDSTRING,',')-1));
			TAP_COUNT:=INSTR(APPENDSTRING,',')+1 ;
			APPENDSTRING:=SUBSTR(APPENDSTRING,TAP_COUNT,LENGTH(APPENDSTRING));
			TAPCODE := NVL(RESULT_TAPCODE,'Null') ;
		BEGIN	
			select HUR_ENABLED into hurenabled from ROAMER_PARTNER where tap_code=TAPCODE ;
			select INBOUND_SEQUENCE_NUMBER into inboundseqnumber from ROAMER_PARTNER where tap_code=TAPCODE ;
			select EMAIL_ID  into EMAILID from ROAMER_PARTNER where tap_code=TAPCODE ;
			select INBOUND_THRESHOLD into inboundTHRESHOLD from ROAMER_PARTNER where tap_code=TAPCODE ;
			
			--************************ HUR ENABLED OR NOT *******************
			IF hurenabled = 1
			THEN
				
			LINE_COUNT := 0 ;
		
			SEQ_NUMBER := LPAD(NVL(inboundseqnumber, '0000' || TO_CHAR(SYSDATE, 'YYYY')) + 10000, 8, 0) ;
			IF RESET_SEQUANCE (SEQ_NUMBER) = 1 THEN
				SEQ_NUMBER := LPAD ('0001' || TO_CHAR(SYSDATE, 'YYYY'), 8, 0) ;
			END IF ;
			--UPDATE ROAMER_PARTNER SET INBOUND_SEQUENCE_NUMBER = SEQ_NUMBER WHERE TAP_CODE = TAPCODE ;
	
			FILENAME:='HUR_ALLZAFCC' || TAPCODE || SEQ_NUMBER || '.csv' || '~' || EMAILID ;

			--OUT_FILE := UTL_FILE.FOPEN ('$filePath', FILENAME, 'w') ; 


			dbms_output.put_line('FILENAME '||FILENAME) ;
			INSERT INTO TMP_REP_DATA SELECT SERVICE, IMSI, 
				MIN(TIME_STAMP), MIN(TIME_STAMP), 
				MAX(TIME_STAMP), MAX(TIME_STAMP),  
				SUM(DURATION), COUNT(*), NULL, SUM(VALUE)
			FROM INROAMER_HUR 
			WHERE TIME_STAMP BETWEEN START_DATE AND END_DATE AND INROAMER_HUR.ROAMER_PARTNER = TAPCODE
			GROUP BY IMSI, SERVICE 
			HAVING SUM(VALUE) >= inboundTHRESHOLD ;
			
			INSERT INTO TMP_REP_GPRS_DATA SELECT GPRS_SERVICE, IMSI, 
				MIN(TIME_STAMP), MIN(TIME_STAMP), 
				MAX(TIME_STAMP), MAX(TIME_STAMP),  
				SUM(DURATION), COUNT(*),SUM(VOLUME), SUM(VALUE)
			FROM GPRS_INROAMER_HUR 
			WHERE TIME_STAMP BETWEEN START_DATE AND END_DATE AND GPRS_INROAMER_HUR.ROAMER_PARTNER = TAPCODE AND SESSION_STATUS=1
			GROUP BY IMSI, GPRS_SERVICE 
			HAVING SUM(VALUE) >= inboundTHRESHOLD ;
		------------------------------------------------------######## SET A OF HUR ##########------------------------------------------------------------------------------		
			PRINT_LINE ('R') ;
			PRINT_LINE ('R,SENDER,RECIPIENT,SEQUENCE NO,THRESHOLD,DATE AND TIME OF ANALYSIS,DATE AND TIME OF REPORT CREATION') ;
			PRINT_LINE ('R') ;
			PRINT_LINE ('H,ZAFCC,' || TAPCODE || ',' || SEQ_NUMBER || ',' || inboundTHRESHOLD || ',' || TO_CHAR(SYSDATE, 'YYYYMMDDHHMISS') || ',' || TO_CHAR(SYSDATE, 'YYYYMMDDHHMISS')) ;
			PRINT_LINE ('R') ;

			SELECT COUNT(*) INTO Temp FROM TMP_REP_DATA ;
			SELECT COUNT(*) INTO Temp1 FROM TMP_REP_GPRS_DATA ;

			IF Temp > 0 or Temp1 >0 THEN
				PRINT_LINE ('R,IMSI,DATE FIRST EVENT,TIME FIRST EVENT,DATE LAST EVENT,TIME LAST EVENT,DC(HHHMMSS),NC,VOLUME,SDR') ;
			ELSE
				PRINT_LINE ('R,BEGINING OF THE OBSERVATION PERIOD,END OF THE OBSERVATION PERIOD') ;
				PRINT_LINE ('N,' || TO_CHAR(START_DATE, 'YYYYMMDD') || ',' || TO_CHAR(END_DATE, 'YYYYMMDD')) ;
			END IF ;
	
			PRINT_RECORDS (TAPCODE) ;
			
			IF REC_COUNT <= 0 THEN
				PRINT_LINE ('R') ; PRINT_LINE ('R') ;
				PRINT_LINE ('R,END OF REPORT') ;
				PRINT_LINE ('R') ;
				PRINT_LINE ('R,NONE OF THE IMSIS EXCEEDED THE AGREED THRESHOLD ON THIS PMN') ;
				PRINT_LINE ('R') ;
			ELSE
				PRINT_LINE ('R') ; PRINT_LINE ('R') ; PRINT_LINE ('R') ;
				PRINT_LINE ('R,IMSI,DATE FIRST EVENT,DATE LAST EVENT,DESTINATION OF EVENTS,NC,DC(HHHMMSS),SDR') ;
				
---------------------------------------------################## SET C OF HUR ################ -----------------------------------------------------------


				FOR GROUPED_CDRS IN ROAMER_PARTNER_SUB_CDRS LOOP
					PRINT_LINE ('A,' ||
							GROUPED_CDRS.IMSI || ',' ||
							TO_CHAR(GROUPED_CDRS.DATE_FIRST_EVENT, 'YYYYMMDD') || ',' ||
							TO_CHAR(GROUPED_CDRS.DATE_LAST_EVENT, 'YYYYMMDD') || ',' ||
							GROUPED_CDRS.COUNTRY_CODE || ',' || GROUPED_CDRS.NC || ',' || 
							DC_HHHMMSS(GROUPED_CDRS.DC) || ',' || GROUPED_CDRS.SDR) ;
				END LOOP ;

				FOR GROUPED_CDRS IN ROAMER_PARTNER_SMS_CDRS LOOP
				select sms_treaty into smstreaty from roamer_partner where tap_code=TAPCODE;
					IF smstreaty=1
					THEN
					PRINT_LINE ('B,' ||
							GROUPED_CDRS.IMSI || ',' ||
							TO_CHAR(GROUPED_CDRS.DATE_FIRST_EVENT, 'YYYYMMDD') || ',' ||
							TO_CHAR(GROUPED_CDRS.DATE_LAST_EVENT, 'YYYYMMDD') || ',' ||
							GROUPED_CDRS.COUNTRY_CODE || ',' || GROUPED_CDRS.NC ||
							 ',,' || GROUPED_CDRS.SDR) ;
					END IF ;		
				END LOOP ;

--------------------------------------------################### SET D OF HUR #####################---------------------------------------------------


				PRINT_LINE ('R') ; PRINT_LINE ('R') ;
				PRINT_LINE ('R,END OF REPORT') ;
				DELETE FROM TMP_REP_GPRS_DATA ;
				DELETE FROM TMP_REP_DATA ;
				DELETE FROM TMP_REP_SEC_C_DATA ;
				DELETE FROM TMP_REP_SEC_C_GPRS_DATA ;
			END IF ;
			PRINT_LINE ('R') ;
			PRINT_LINE ('T,' || LINE_COUNT) ;
			UTL_FILE.FCLOSE (OUT_FILE) ;
			UPDATE ROAMER_PARTNER SET INBOUND_LAST_PROCESSED_TIME = SYSDATE WHERE TAP_CODE = TAPCODE ;
			END IF ;
			ACCEPTED_TAPCODES_COUNT := ACCEPTED_TAPCODES_COUNT +1 ;
			ACCEPTED_TAPCODES := ACCEPTED_TAPCODES||','||TAPCODE ;

	EXCEPTION 
		WHEN NO_DATA_FOUND 
		THEN
				REJECTED_TAPCODE_COUNT := REJECTED_TAPCODE_COUNT+1 ;
				REJECTED_TAPCODES := REJECTED_TAPCODES||','||TAPCODE ;
	
         WHEN UTL_FILE.INVALID_OPERATION
         THEN
                 UTL_FILE.FCLOSE(LOG_FILE);
	END ;
		EXIT WHEN (TAP_COUNT=0);
	END LOOP;
	END;

	BEGIN
		--LOG_FILE := UTL_FILE.FOPEN('RANGERHOME_LOG','HURReportGen.log','A') ;
		select DIRECTORY_PATH into dirpath from all_directories where DIRECTORY_NAME='$filePath' and rownum <2;
		FILENAME:='' ;
		--***********************************************************
		-- Validate Input Date Range
		--***********************************************************
		Temp := VALIDATE_DATE ('$StartDate', START_DATE) ;
		IF Temp = 1 THEN
			START_DATE := SYSDATE ;
			dbms_output.put_line('VALIDATE_DATE Start failed') ;
			RETURN ;
		END IF;
		Temp := VALIDATE_DATE ('$EndDate', END_DATE) ;
		IF Temp = 1 THEN
			START_DATE := SYSDATE ;
			dbms_output.put_line('VALIDATE_DATE enddate failed') ;
			RETURN ; 
		END IF;
		IF START_DATE > END_DATE
		then
			insert into hur_output_store values(1, 'LOG', '	******************* HUR REPORT GENERATION  Log ******************');
			insert into hur_output_store values(2, 'LOG', '') ;
			insert into hur_output_store values(3, 'LOG', 'start time : $LOG_START_TIME            End Time : $LOG_START_TIME\n') ;
			insert into hur_output_store values(4, 'LOG', '') ;
			insert into hur_output_store values(5, 'LOG', 'valid End Date : End Date Should Be Greater Than Start Date\n') ;
			insert into hur_output_store values(6, 'LOG', 'ease Provide Valid End Date\n') ;
			insert into hur_output_store values(7, 'LOG', '----------------------------------------------------------------\n\n') ;
			--UTL_FILE.FCLOSE(LOG_FILE);
			return ;
		END IF ;	
		--***********************************************************
		-- For each roamer partner collect the summary of cdr's
		--***********************************************************
		--####################################################################
	IF  GIVEN_TAPCODES <> 'ALL' 
	THEN
		SPLIT_TAPCODES (GIVEN_TAPCODES) ;		
	--######################################################################################
	ELSE	
		SELECT COUNT(*) INTO TOTAL_TAPCODE_COUNT FROM ROAMER_PARTNER ;
		FOR ROAMER_PARTNER_REC IN ROAMER_PARTNER_LIST LOOP
			IF ROAMER_PARTNER_REC.HUR_ENABLED = 1
			THEN
			LINE_COUNT := 0 ;
		
			SEQ_NUMBER := LPAD(NVL(ROAMER_PARTNER_REC.INBOUND_SEQUENCE_NUMBER, '0000' || TO_CHAR(SYSDATE, 'YYYY')) + 10000, 8, 0) ;
			IF RESET_SEQUANCE (SEQ_NUMBER) = 1 THEN
				SEQ_NUMBER := LPAD ('0001' || TO_CHAR(SYSDATE, 'YYYY'), 8, 0) ;
			END IF ;
			UPDATE ROAMER_PARTNER SET INBOUND_SEQUENCE_NUMBER = SEQ_NUMBER WHERE TAP_CODE = ROAMER_PARTNER_REC.TAP_CODE ;

			FILENAME :='HUR_ALLZAFCC' || ROAMER_PARTNER_REC.TAP_CODE || SEQ_NUMBER || '.csv' || '~' || ROAMER_PARTNER_REC.EMAIL_ID ;

			--OUT_FILE := UTL_FILE.FOPEN ('$filePath', FILENAME, 'w') ; 
			insert into hur_output_store values(rownumt, '$filePath', 'FILENAME='||dirpath||'/'||FILENAME)  ;

			INSERT INTO TMP_REP_DATA SELECT SERVICE, IMSI, 
				MIN(TIME_STAMP), MIN(TIME_STAMP), 
				MAX(TIME_STAMP), MAX(TIME_STAMP),  
				SUM(DURATION), COUNT(*), NULL, SUM(VALUE)
			FROM INROAMER_HUR 
			WHERE TIME_STAMP BETWEEN START_DATE AND END_DATE AND INROAMER_HUR.ROAMER_PARTNER = ROAMER_PARTNER_REC.TAP_CODE
			GROUP BY IMSI, SERVICE 
			HAVING SUM(VALUE) >= ROAMER_PARTNER_REC.INBOUND_THRESHOLD ;


			INSERT INTO TMP_REP_GPRS_DATA SELECT GPRS_SERVICE, IMSI, 
				MIN(TIME_STAMP), MIN(TIME_STAMP), 
				MAX(TIME_STAMP), MAX(TIME_STAMP),  
				SUM(DURATION), COUNT(*),SUM(VOLUME), SUM(VALUE)
			FROM GPRS_INROAMER_HUR 
			WHERE TIME_STAMP BETWEEN START_DATE AND END_DATE AND GPRS_INROAMER_HUR.ROAMER_PARTNER = ROAMER_PARTNER_REC.TAP_CODE AND SESSION_STATUS=1
			GROUP BY IMSI, GPRS_SERVICE 
			HAVING SUM(VALUE) >= ROAMER_PARTNER_REC.INBOUND_THRESHOLD ;
				
	----------------------------------------------############### SET A OF HUR ################---------------------------------------------			
			PRINT_LINE ('R') ;
			PRINT_LINE ('R,SENDER,RECIPIENT,SEQUENCE NO,THRESHOLD,DATE AND TIME OF ANALYSIS,DATE AND TIME OF REPORT CREATION') ;
			PRINT_LINE ('R') ;
			PRINT_LINE ('H,ZAFCC,' || ROAMER_PARTNER_REC.TAP_CODE || ',' || SEQ_NUMBER || ',' || ROAMER_PARTNER_REC.INBOUND_THRESHOLD || ',' || TO_CHAR(SYSDATE, 'YYYYMMDDHHMISS') || ',' || TO_CHAR(SYSDATE, 'YYYYMMDDHHMISS')) ;
			PRINT_LINE ('R') ;

			SELECT COUNT(*) INTO Temp FROM TMP_REP_DATA ;
			SELECT COUNT(*) INTO Temp1 FROM TMP_REP_GPRS_DATA ;

			IF Temp > 0 or Temp1 > 0 THEN
				PRINT_LINE ('R,IMSI,DATE FIRST EVENT,TIME FIRST EVENT,DATE LAST EVENT,TIME LAST EVENT,DC(HHHMMSS),NC,VOLUME,SDR') ;
			ELSE
				PRINT_LINE ('R,BEGINING OF THE OBSERVATION PERIOD,END OF THE OBSERVATION PERIOD') ;
				PRINT_LINE ('N,' || TO_CHAR(START_DATE, 'YYYYMMDD') || ',' || TO_CHAR(END_DATE, 'YYYYMMDD')) ;
			END IF ;
	
			PRINT_RECORDS (ROAMER_PARTNER_REC.TAP_CODE) ;
			
			IF REC_COUNT <= 0 THEN
				PRINT_LINE ('R') ; PRINT_LINE ('R') ;
				PRINT_LINE ('R,END OF REPORT') ;
				PRINT_LINE ('R') ;
				PRINT_LINE ('R,NONE OF THE IMSIS EXCEEDED THE AGREED THRESHOLD ON THIS PMN') ;
				PRINT_LINE ('R') ;
			ELSE
				PRINT_LINE ('R') ; PRINT_LINE ('R') ; PRINT_LINE ('R') ;
				PRINT_LINE ('R,IMSI,DATE FIRST EVENT,DATE LAST EVENT,DESTINATION OF EVENTS,NC,DC(HHHMMSS),SDR') ;
				
-----------------------------------------------################### SET C OF HUR ##################---------------------------------------

				FOR GROUPED_CDRS IN ROAMER_PARTNER_SUB_CDRS LOOP
					PRINT_LINE ('A,' ||
							GROUPED_CDRS.IMSI || ',' ||
							TO_CHAR(GROUPED_CDRS.DATE_FIRST_EVENT, 'YYYYMMDD') || ',' ||
							TO_CHAR(GROUPED_CDRS.DATE_LAST_EVENT, 'YYYYMMDD') || ',' ||
							GROUPED_CDRS.COUNTRY_CODE || ',' || GROUPED_CDRS.NC || ',' || 
							DC_HHHMMSS(GROUPED_CDRS.DC) || ',' || GROUPED_CDRS.SDR) ;
				END LOOP ;
				FOR GROUPED_CDRS IN ROAMER_PARTNER_SMS_CDRS LOOP
				select sms_treaty into smstreaty from roamer_partner where tap_code=ROAMER_PARTNER_REC.TAP_CODE;
					IF smstreaty=1
					THEN
					PRINT_LINE ('B,' ||
							GROUPED_CDRS.IMSI || ',' ||
							TO_CHAR(GROUPED_CDRS.DATE_FIRST_EVENT, 'YYYYMMDD') || ',' ||
							TO_CHAR(GROUPED_CDRS.DATE_LAST_EVENT, 'YYYYMMDD') || ',' ||
							GROUPED_CDRS.COUNTRY_CODE || ',' || GROUPED_CDRS.NC ||
							 ',,' || GROUPED_CDRS.SDR) ;
					END IF ;		
				END LOOP ;

---------------------------------------------###################### SET D OF HUR #####################-----------------------------


				PRINT_LINE ('R') ; PRINT_LINE ('R') ;
				PRINT_LINE ('R,END OF REPORT') ;
				DELETE FROM TMP_REP_GPRS_DATA ;
				DELETE FROM TMP_REP_DATA ;
				DELETE FROM TMP_REP_SEC_C_DATA ;
				DELETE FROM TMP_REP_SEC_C_GPRS_DATA ;
			END IF ;
			PRINT_LINE ('R') ;
			PRINT_LINE ('T,' || LINE_COUNT) ;
			--UTL_FILE.FCLOSE (OUT_FILE) ;
			UPDATE ROAMER_PARTNER SET INBOUND_LAST_PROCESSED_TIME = SYSDATE WHERE TAP_CODE = ROAMER_PARTNER_REC.TAP_CODE ;
		END IF ;
		END LOOP ;
	END IF ;
	if '$id' != '0'  
	then
		update hur_jobs set output_path='$FILENAME' where id='$id' ;
		commit ;
	end if ;	
			REJECTED_TAPCODES := NVL(SUBSTR(REJECTED_TAPCODES,2),'NULL') ;
			ACCEPTED_TAPCODES := NVL(SUBSTR(ACCEPTED_TAPCODES,2),'ALL') ;
			SELECT DECODE(ACCEPTED_TAPCODES_COUNT,0,TOTAL_TAPCODE_COUNT,ACCEPTED_TAPCODES_COUNT) INTO ACCEPTED_TAPCODES_COUNT  FROM DUAL  ;
			insert into hur_output_store values(1, 'LOG','	******************* HUR REPORT GENERATION  Log ******************');
			insert into hur_output_store values(2, 'LOG','') ;
			LOG_END_TIME := TO_CHAR(SYSDATE,'DD/MM/YYYY HH24:MI:SS') ;
			insert into hur_output_store values(3, 'LOG','Start time : $LOG_START_TIME            End Time : '||LOG_END_TIME) ;
			insert into hur_output_store values(4, 'LOG','') ;
			insert into hur_output_store values(5, 'LOG','Processed TAPCodes count  	 : '||TOTAL_TAPCODE_COUNT) ;
			insert into hur_output_store values(6, 'LOG','Accepted  TAPCodes count   	 : '||ACCEPTED_TAPCODES_COUNT ) ;
			insert into hur_output_store values(7, 'LOG','Rejected  TAPCodes count  	 : '||REJECTED_TAPCODE_COUNT) ;
			insert into hur_output_store values(8, 'LOG','Accepted  TAPCodes   	  	 : '||ACCEPTED_TAPCODES) ;
			insert into hur_output_store values(9, 'LOG','Rejected  TAPCodes   	  	 : '||REJECTED_TAPCODES) ;
			insert into hur_output_store values(10, 'LOG','Report Generated For The Period '||to_char(START_DATE,'dd/mm/yyyy')||' - '||to_char(END_DATE,'dd/mm/yyyy')) ;
			insert into hur_output_store values(11, 'LOG','----------------------------------------------------------------') ;
			--UTL_FILE.FCLOSE(LOG_FILE);
END ;
/

set lines 3000;
set trimspool on;
set pages 0;
set feedback off;
spool $TAPFILE_NAME ;
select output_line from hur_output_store where output_type='$filePath' order by seq_no asc;
spool off
spool $RANGERHOME/LOG/HURReportGen.log
select output_line from hur_output_store where output_type='LOG' order by seq_no asc;
EOF

if [ "$Whom" != "ALL" ]
then
	modified_file_name=`echo $TAPFILE_NAME | sed 's/\^/@/g'`
	echo $modified_file_name
	mv $TAPFILE_NAME $modified_file_name
else
	outfilename=""
	while read line
	do
		echo $line | grep FILENAME >/dev/null
		if [ $? -eq 0 ]
		then
			outfilename=`echo $line | cut -f2 -d"="`
		else
			echo $line >>$outfilename
		fi
	done<$TAPFILE_NAME
	
fi
}

SendMail ()
{
	for File in `ls $1/*.csv~*`
	do
		File=`basename $File`
		FileName=`echo $File | cut -d'~' -f1`
		E_MailID=`echo $File | cut -d'~' -f2 |sed 's/\^/@/g'`
		mv  $1/$File $1/$FileName
		grep "^T,14$" $1/$FileName > /dev/null
		if [ $? -ne 0 ]
        then
		#uuencode $1/$FileName $FileName 2>/dev/null | mailx -s "HUR From CellC-SouthAfrica - $FileName" $E_MailID,$HUR_ADMIN_EMAIL_ID
		mailx -a $1/$FileName -s "HUR From CellC-SouthAfrica - $FileName" $E_MailID,$HUR_ADMIN_EMAIL_ID<<EOF
			HUR From Cellc
EOF
		fi
		if [ "$DeleteFile" -ne "0" ]; then
			rm -f $1/$FileName
		fi
	done 2> /dev/null
}
StartDate=""
EndDate=""
Whom=""


if [ "$1" == "" ]
then
	Whom="ALL" 
else
	Whom=$1 
fi 
if [ "$2" == "None" -o "$2" == "" ]
then
	d=`date +%d`
    m=`date +%m`
    y=`date +%Y`
	get_one_day_before_specified_date $d $m $y	
else
	StartDate=$2 ;
	EndDate=$StartDate ;
fi
if [ "$3" != "None" -a "$3" != "" ]
then
	EndDate=$3 ;
fi

StartDate=$StartDate"00:00:00"
EndDate=$EndDate"23:59:59"
#echo -e "";
#echo -e "Start Date===>$StartDate";
#echo -e "End Date===>$EndDate";
#echo -e "For Whom Report to be sent: $Whom";
AutomatedReportDir=$NIKIRAROOT/FMSData/HUROutput
ManualReportDir=$NIKIRAROOT/FMSData/HUROutput/ManualOutput

GetConfigEntry "HUR_CLEANUP_INTERVAL"
if [ $? -eq 0 ]; then
	CleanupDuration=$TempConfigValue
else
	logger "GenerateHURInStandardFormat - Unable To Find Config Entry HUR_CLEANUP_INTERVAL [Defaulting To 8] ..."
fi

GetConfigEntry "DELETE_FILES_AFTER_MAILING"
if [ $? -eq 0 ]; then
	DeleteFile=$TempConfigValue
else
	logger "GenerateHURInStandardFormat - Unable To Find Config Entry DELETE_FILES_AFTER_MAILING [Defaulting To False] ..."
fi

GetConfigEntry "HUR_ADMIN_EMAIL_ID"
if [ $? -eq 0 ]; then
	HUR_ADMIN_EMAIL_ID=$TempConfigValue
else
	logger "GenerateHURInStandardFormat - Unable To Find Config Entry HUR_ADMIN_EMAIL_ID ..."
	exit 5 ;
fi

if [ "$4" != "" ]
then
	Initialize 
	GenerateReport "$ManualReportDir" $4
	CleanUp
	if [ "$1" == "ALL" ]
	then
		SendMail "$ManualReportDir"
	fi	
else
	Initialize 
	GenerateReport "$AutomatedReportDir"
	CleanUp 
	SendMail "$AutomatedReportDir"
fi
