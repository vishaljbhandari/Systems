#!/bin/bash
MAGIC_NUMBER=13323
function CheckIfAlreadyRunning()
{
	if [ -f $HOME/.script_lock_$MAGIC_NUMBER  ]
	then
		echo -e "[ERROR] SCRIPT [$0] WITH MAGIC NUMBER[$MAGIC_NUMBER] ALREADY RUNNING, HENCE EXITING"
		exit 22	
	fi
	# To Remove Lock, Run : rm $HOME/.script_lock_$MAGIC_NUMBER
	touch $HOME/.script_lock_$MAGIC_NUMBER
}
CheckIfAlreadyRunning
# There must be an uninitialize function to clear
# Or Add "Deletion of this lock file in any such uninitialize function
function ScriptDestructorFunction(){
	rm $HOME/.script_lock_$MAGIC_NUMBER
}
trap ScriptDestructorFunction EXIT
#################################################################################
