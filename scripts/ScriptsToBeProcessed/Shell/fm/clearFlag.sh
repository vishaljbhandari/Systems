#!/bin/bash
. $RUBY_UNIT_HOME/Scripts/configuration.sh
. $RANGERHOME/sbin/rangerenv.sh


clearFlag()
{
filename=$1
rm 'tmp/flag_'$filename
}

main ()
{
    if [ $# -eq 1 ]
    then
        clearFlag $1
    else
        echo "Improper Usage: First argument should be the filename"
        exit 3
    fi
}

main $*
	
