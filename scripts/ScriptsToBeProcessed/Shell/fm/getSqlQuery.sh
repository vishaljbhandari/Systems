#!/bin/bash

if [ $# -ne 1 ];then

            echo "Usage: oraproc <Unix Process ID>"

            exit

fi

 

unix_pid=$1

 

sqlplus -s -l / as sysdba <<EOF

set head off feed off time off timing off

set serveroutput on lines 10000

DECLARE

            stmt VARCHAR2(32000) := '';

            report BOOLEAN := TRUE;

            SPACECHAR CONSTANT CHAR(1) := ' ';

            TABLEN CONSTANT NUMBER(2) := 10;

BEGIN

            dbms_output.enable(99999);

            FOR rec IN (

                        SELECT  s.sid, s.username, s.osuser, s.machine,

                                                nvl(s.module, s.program) prog, s.event, t.sql_text

                        FROM    v\$process p

                        JOIN    v\$session s ON s.paddr = p.addr

                        LEFT JOIN v\$sqltext_with_newlines t

                                                ON       t.address = s.sql_address

                                                AND     t.hash_value = s.sql_hash_value

                        WHERE   p.spid = ${unix_pid}

                        ORDER BY t.piece ) LOOP

 

                        stmt := stmt || rec.SQL_TEXT;

                        IF report THEN

                                    DBMS_OUTPUT.PUT_LINE(RPAD('SID:', TABLEN, spacechar) || rec.sid);

                                    DBMS_OUTPUT.PUT_LINE(RPAD('USERNAME:', TABLEN, spacechar) || rec.username);

                                    DBMS_OUTPUT.PUT_LINE(RPAD('OSUSER:', TABLEN, spacechar) || rec.osuser);

                                    DBMS_OUTPUT.PUT_LINE(RPAD('MACHINE:', TABLEN, spacechar) || rec.machine);

                                    DBMS_OUTPUT.PUT_LINE(RPAD('PROGRAM:', TABLEN, spacechar) || rec.prog);

                                    DBMS_OUTPUT.PUT_LINE(RPAD('EVENT:', TABLEN, spacechar) || rec.event);

                                    DBMS_OUTPUT.PUT_LINE('---------------------------------------');

                                    report := FALSE;

                        END IF;

            END LOOP;

            IF NOT report THEN

                        IF LENGTH(stmt) > 0 THEN

                                    DBMS_OUTPUT.PUT_LINE(stmt);

                        ELSE

                                    DBMS_OUTPUT.PUT_LINE('No current running statement');

                        END IF;

            ELSE

                        DBMS_OUTPUT.PUT_LINE('Unix PID ${unix_pid} not found');

            END IF;

END;

/

EXIT

EOF

 

