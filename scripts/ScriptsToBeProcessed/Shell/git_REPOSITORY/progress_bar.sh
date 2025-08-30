#!/bin/bash
#################################################################################
#
#
#
#################################################################################
# PROGRESS BAR
#
#
#################################################################################

PROGRESS_MESSAGE="PROGRESS"
FILL_CHARACTER=#
function ProgressBar {
        let _progress=(${1}*100/${2}*100)/100
        let _done=(${_progress}*4)/10
        let _left=40-$_done
        _fill=$(printf "%${_done}s")
        _empty=$(printf "%${_left}s")
        printf "\r$PROGRESS_MESSAGE : [${_fill// /$FILL_CHARACTER}${_empty// /-}] ${_progress}%%"
}

# PROGRESS INSTANCE 1
_start=1
_end=100
INTERVAL=0.03
for number in $(seq ${_start} ${_end}); do sleep $INTERVAL; ProgressBar ${number} ${_end}; done
printf "\n"

#################################################################################
