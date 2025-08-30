#! /bin/bash
                                                                                                                 
#/*******************************************************************************                                 
#*  Copyright (c) Subex Limited 2014. All rights reserved.                      *                                 
#*  The copyright to the computer program(s) herein is the property of Subex    *                                 
#*  Limited. The program(s) may be used and/or copied with the written          *                                 
#*  permission from Subex Limited or in accordance with the terms and           *                                 
#*  conditions stipulated in the agreement/contract under which the program(s)  *                                 
#*  have been supplied.                                                         *                                 
#********************************************************************************/ 


sqlplus -s /nolog << eof
CONNECT_TO_SQL

set serveroutput off;
set feedback off;
set lines 120;
set colsep ,
set pages 0;
set heading off;

DROP TABLE OUTROAMER_GPRS_VPMN_RATES;

quit;
eof
