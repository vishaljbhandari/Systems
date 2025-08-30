#----------------------------
# THIS SCRIPT CREATES MULTIPLE FILES FOR TESTING PURPOSE IN GIVEN DIRECTORY
# THEN TOUCH THOSE FILE INTO SUCCESS DIRECTORY
#---------------------------
if [ $# -lt 3 ]
then
        echo "Usage: scriptlauncher $0 PATH FILE COUNT"
        exit 1
fi

dir=$1
f=1
s=3
t=7
if [ -d  $dir ]
then
        if [ ! -d $dir/success ]
        then
                mkdir $dir/success
        fi
        file=$2
        if [ ! -f ./$file$f ]
        then
                echo "FILE NOT FOUND"
                exit 2
        fi
        if [ ! -f ./$file$s ]
        then
                echo "FILE NOT FOUND"
                exit 2
        fi
        if [ ! -f ./$file$t ]
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
                cp ./$file$f $dir/$file$f_$c
                touch $dir/success/$file$f_$c
                cp ./$file$s $dir/$file$s_$c
                touch $dir/success/$file$s_$c
                cp ./$file$t $dir/$file$t_$c
                touch $dir/success/$file$t_$c
                c=`expr $c + 1`
        done
else
        echo "SPECIFIED DIRECTORY NOT FOUND"
fi
