#----------------------------
# THIS SCRIPT CREATES MULTIPLE FILES FOR TESTING PURPOSE IN GIVEN DIRECTORY
# THEN TOUCH THOSE FILE INTO SUCCESS DIRECTORY
#---------------------------
if [ $# -lt 3 ]
then
        echo "Usage: source $0 PATH FILE COUNT"
        exit 1
fi

dir=$1
if [ -d  $dir ]
then
        if [ ! -d $dir/success ]
        then
                mkdir $dir/success
        fi
        file=$2
        if [ ! -f ./$file ]
        then
                echo "FILE NOT FOUND"
                exit 2
        fi
        if [ ! -z $3 ]
        then
                limit=1000
        else
                limit=$3
        fi
        c=0
        while [ $c -le $limit ]
        do
                cp ./$file $dir/$file$c
                touch $dir/success/$file$c
                c=`expr $c + 1`
        done
else
        echo "SPECIFIED DIRECTORY NOT FOUND"
fi
