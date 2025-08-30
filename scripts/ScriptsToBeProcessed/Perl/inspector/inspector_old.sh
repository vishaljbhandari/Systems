#!/bin/bash
###################################
###################################
# STAT MASTER REPORT
# AUTHOR  : VISHAL BHANDARI 
###################################
CURR_DIR=`pwd`
cd $CURR_DIR
INSPECTOR_CODE=".InspectorRoot"
CURRENTDATE=`date +"%Y%m%d"`
SCRIPT_NAME="inspector.sh"

function finish {
	touch $CURR_DIR/.$SCRIPT_NAME.$CURRENTDATE.lock
	rm $CURR_DIR/.$SCRIPT_NAME.$CURRENTDATE.lock
	echo -e "[99999] Exiting Script, Deleting Lock ."$SCRIPT_NAME.$CURRENTDATE.lock
	
	# Cleaning Section

}
trap finish EXIT
touch $CURR_DIR/.$SCRIPT_NAME.tmp.lock
running_count=`ls $CURR_DIR/.$SCRIPT_NAME.*.lock | wc -l`
if [ $running_count -gt 1 ] ; then
	echo -e "[ERROR] "$SCRIPT_NAME" already running, cant start"
    exit 99
fi

touch $CURR_DIR/.$SCRIPT_NAME.$CURRENTDATE.lock

echo -e "[00001] Welcome to Inspector Script"
echo -e "[00003] Inspector Case Id C"$CURRENTDATE
echo -e "[00002] Current Directory "$CURR_DIR
if [ ! -f $CURR_DIR/$INSPECTOR_CODE ] ; then
	echo -e "[ERROR] INVALID INSPECTOR DIRECTORY, Script Must Be Running From The Inspector Rood Directory"
	exit 999
fi
mkdir -p $CURR_DIR"/tmp"
LOG_DIR=$CURR_DIR"/logs"
if [ ! -d $LOG_DIR ] ; then
	echo -e "[ERROR] Missing Log Directory "$LOG_DIR
	exit 998
fi
echo -e "[00003] Log Directory "$LOG_DIR

CASE_ROOT=$CURR_DIR
CASE="cases"
CASE_DIR=$CASE_ROOT/$CASE"/C"$CURRENTDATE
if [ ! -d $CASE_DIR ] ; then
	mkdir -p $CASE_DIR $CASE_DIR/git $CASE_DIR/stat $CASE_DIR/report $CASE_DIR/src $CASE_DIR/hash 2>/dev/null
	rm $CASE_DIR/git/* $CASE_DIR/stat/* $CASE_DIR/report/* $CASE_DIR/src/* $CASE_DIR/hash/* 2>/dev/null
fi
echo -e "[00004] Current Case Directory : "$CASE_DIR

LANDING_DIR=$CURR_DIR"/landing"
if [ ! -d $LANDING_DIR ] ; then
	echo -e "[ERROR] Missing Landing Directory "$LANDING_DIR
	exit 998
fi
echo -e "[00005] Landing Directory : "$LANDING_DIR
dos2unix $LANDING_DIR/* 2>&1

TEMPLATE_DIR=$CURR_DIR"/templates"
if [ ! -d $TEMPLATE_DIR ] ; then
	echo -e "[ERROR] Missing Template Directory "$TEMPLATE_DIR
	exit 997
fi
echo -e "[00006] Template Directory : "$TEMPLATE_DIR
dos2unix $TEMPLATE_DIR/* 2>&1

CONF_DIR=$CURR_DIR"/conf"
if [ ! -d $CONF_DIR ] ; then
	echo -e "[ERROR] Missing Conf Directory "$CONF_DIR
	exit 996
fi
echo -e "[00007] Conf Directory : "$CONF_DIR
dos2unix $CONF_DIR/* 2>&1

CONF_FILE=$CURR_DIR"/conf/inspector.conf"
if [ ! -f $CONF_FILE ] ; then
	echo -e "[ERROR] Missing Config File "$CONF_FILE
	exit 995
fi
echo -e "[00010] Config File : "$CONF_FILE


stat_loader()
{
	cd $CURR_DIR
	STAT_MASTER_SCRIPT="stat_master.sh"
	echo -e "[STAT1] Starting Stat Master Script ["$STAT_MASTER_SCRIPT"] Log ["$LOG_DIR"/stat-inspector-"$CURRENTDATE".log]"
	. $CURR_DIR/$STAT_MASTER_SCRIPT > $LOG_DIR/stat-inspector-$CURRENTDATE.log 2> $LOG_DIR/stat-inspector-$CURRENTDATE.log.err &
	stat_inspector_process_id=$!
	wait $stat_inspector_process_id
	SSCRIPT_RUN_RESULT=$?

	if [ $SSCRIPT_RUN_RESULT -eq 0 ] ; then
		echo -e "[00009] Stat Master Script Successfully Executed"
	else 
		echo -e "[ERROR] Stat Master Script Failed with error code ["$SSCRIPT_RUN_RESULT"]"
	fi
	cd $CURR_DIR
}

echo -e "[00011] Running Stat Loader"
stat_loader












pre_inspector()
{
	cd $CURR_DIR
	GIT_MASTER_SCRIPT="git_master.sh"
	echo -e "[GIT01] Starting Git Master Script ["$GIT_MASTER_SCRIPT"] Log ["$LOG_DIR"/git-inspector-"$CURRENTDATE".log]"
	. $CURR_DIR/$GIT_MASTER_SCRIPT > $LOG_DIR/git-inspector-$CURRENTDATE.log 2> $LOG_DIR/git-inspector-$CURRENTDATE.log.err &
	git_inspector_process_id=$!
	wait $git_inspector_process_id
	GSCRIPT_RUN_RESULT=$?
	
	if [ $GSCRIPT_RUN_RESULT -eq 0 ] ; then
		echo -e "[00009] Git Master Script Successfully Executed"
	else 
		echo -e "[ERROR] Git Master Script Failed with error code ["$GSCRIPT_RUN_RESULT"]"
	fi
	cd $CURR_DIR
	
}
echo -e "[00011] Running Pre Inspector"
pre_inspector

cd $CURR_DIR
sed -i -e 's/\r$//' *.sh && chmod 755  *.sh
sed -i -e 's/\r$//' *.pl && chmod 755  *.pl

FINAL_REPORT_FILE=$CURR_DIR/dp_report_$CURRENTDATE.html
touch $FINAL_REPORT_FILE
rDATE=`date +"%Y-%m-%d"`
cp $CURR_DIR/templates/header $FINAL_REPORT_FILE
perl -pi.back -e 's/__XXXXXX__/$rDATE/g;' $FINAL_REPORT_FILE
INTERMEDIATE_FILE=$CURR_DIR/dp_report_inter
touch $INTERMEDIATE_FILE
> $INTERMEDIATE_FILE
INSPECTOR_SCRIPT="inspector.pl"
echo -e "[00008] Starting Inspector Script [inspector.pl]"
perl $INSPECTOR_SCRIPT $CONF_FILE > $LOG_DIR/inspector-$CURRENTDATE.log &
inspector_process_id=$!
wait $inspector_process_id
SCRIPT_RUN_RESULT=$?

if [ $SCRIPT_RUN_RESULT -eq 0 ] ; then
	echo -e "[00009] Inspector Script Successfully Executed"
	cat $INTERMEDIATE_FILE >> $FINAL_REPORT_FILE
	cat $CURR_DIR/templates/footer >> $FINAL_REPORT_FILE 
else 
	echo -e "[ERROR] Inspector Script Failed"
	rm $FINAL_REPORT_FILE
	cat $INTERMEDIATE_FILE
fi
echo -e "[00011] Logged run date in logger"
echo $CURRENTDATE >> $CASE_ROOT/$CASE"/.runDetails"
echo -e "[00010] Please check bellow log files"
ls  -ltr $LOG_DIR/*$CURRENTDATE*log