#!/bin/bash
###################################
###################################
# STAT MASTER REPORT
# AUTHOR  : VISHAL BHANDARI 
###################################
CURR_DIR=`pwd`
cd $CURR_DIR
NumericReg='^[0-9]+$'
CURRENTDATE=`date +"%Y%m%d"`
SCRIPT_NAME="cleaner.sh"
if [ $# -ne 2 ] ; then
	echo -e "[ INFO] Usage : "$SCRIPT_NAME" LAST_DAY DAYS_TO_CLEAN"
	exit
fi
iDAYS=$1
iCOUNTER=$2
if ! [[ $iDAYS =~ $NumericReg ]] ; then
	echo -e "[ERROR] Invalid DAYS argument ["$iDAYS"]"
	exit
fi
if ! [[ $iCOUNTER =~ $NumericReg ]] ; then
	echo -e "[ERROR] Invalid COUNTER argument ["$iCOUNTER"]"
	exit
fi
DAYS=$iDAYS
COUNTER=$(( $DAYS + iCOUNTER ))
while [ $DAYS -lt $COUNTER ]; do
	CLEAN_DATE1=$(date -d "$date -$DAYS days" +"%Y%m%d")
	CLEAN_DATE=`echo "${CLEAN_DATE1}" | sed -e 's/^[ \t]*//'`
	if [ -z "$CLEAN_DATE" -a "$CLEAN_DATE" != " " ]; then
		echo -e "Invalid CLEAN_DATE = "$CLEAN_DATE
		exit
	fi
	echo -e "[INFO] Cleanup Process Started With "$DAYS" Days, Clean Date "$CLEAN_DATE 
	rm $CURR_DIR/logs/*$CLEAN_DATE* 2>/dev/null
	rm $CURR_DIR/landing/stat_master.data 2>/dev/null
	rm $CURR_DIR/landing/*$CLEAN_DATE* 2>/dev/null
	grep -v '$CLEAN_DATE##' $CURR_DIR/cases/stat_history.db $CURR_DIR/cases/stat_history.db.tmp 2>/dev/null
	mv $CURR_DIR/cases/stat_history.db.tmp $CURR_DIR/cases/stat_history.db 2>/dev/null 
	rm -rf $CURR_DIR/cases/C$CURR_DIR 2>/dev/null
    ((DAYS++))
done