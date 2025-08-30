#!/bin/bash

. $HOME/Watir/Scripts/configuration.sh
mkdir -p $HOME/Watir/Scripts/tmp

query=$1
query=${query//\[\[EQUALTO\]\]/\=}
query=${query//\[\[OPENBRACKET\]\]/\(}
query=${query//\[\[CLOSEBRACKET\]\]/\)}
query=${query//\[\[WHITESPACE\]\]/ }
query=${query//\[\[SINGLEQUOTE\]\]/\'}
query=${query//\[\[LESSTHAN\]\]/\<}
query=${query//\[\[GREATERTHAN\]\]/\>}
query=${query//\[\[PIPE\]\]/\|}

TMPDIR="$HOME/Watir/Scripts/tmp"
tmpsqlOP=.tmpsqlOP

>$TMPDIR/query_results.lst
. ~/.bash_profile
. ~/.bashrc

if [ $DB_TYPE == "1" ]
then

sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD <<EOF > $tmpsqlOP  2>&1
    set termout off;
    set echo off;
    SET FEEDBACK OFF;
    SET HEADING OFF ;
    SET pagesize 1000 ;
    WHENEVER OSERROR  EXIT 6 ;
    WHENEVER SQLERROR EXIT 5 ;
    SPOOL $TMPDIR/query_results.lst
    $query ;
    SPOOL OFF ;
EOF

else
clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME << EOF > $tmpsqlOP  2>&1 | grep -v "CLPPlus: Version 1.3" | grep -v "Copyright (c) 2009, IBM CORPORATION.  All rights reserved." | grep -v "off" | grep -v "SPOOL" | grep -iv "SELECT" | grep -v "quit" | grep -v "SET" | grep -v ";"
set termout off;
set echo off;
SET FEEDBACK OFF;
SET HEADING OFF ;
SET pagesize 1000 ;
WHENEVER OSERROR  EXIT 6 ;
WHENEVER SQLERROR EXIT 5 ;
SPOOL $HOME/Watir/Scripts/tmp/query_results.lst
$query ;
SPOOL OFF ;
EOF

fi

cat $TMPDIR/query_results.lst | tr -s " " |  tr '\n' ' ' |perl -p -e 's/^ //g'|perl -p -e 's/ $//g' | tr -s '  ' ' '
