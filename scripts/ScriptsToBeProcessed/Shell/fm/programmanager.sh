################  USE THIS SCRIPT TO START PM ONLY IN SOLARIS  #################

if [ $# -ne 1 ]
then
        echo "Error:-  Improper arguments, needs programmanager conf file as argument."
        exit 1
fi

conf_file="$1"

export LD_PRELOAD_64=/lib/sparcv9/libumem.so.1

./programmanager -f $conf_file &

