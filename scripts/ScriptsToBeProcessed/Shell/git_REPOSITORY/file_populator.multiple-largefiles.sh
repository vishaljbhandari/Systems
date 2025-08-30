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
f=_rc1_lf
s=_rc3_lf
t=_rc7_lf
curr=`pwd`
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
                limit=100
        else
                limit=$3
        fi
        c=0
		touch file_list_forpopulator
		touch success_file_list_forpopulator
        while [ $c -le $limit ]
        do
                cp ./$file$f $dir/$file$f$c
				echo $curr/$dir/$file$f$c >> file_list_forpopulator
                touch $dir/success/$file$f$c
				echo $curr/$dir/success/$file$f$c >> success_file_list_forpopulator
                cp ./$file$s $dir/$file$s$c
				echo $curr/$dir/$file$s$c >> file_list_forpopulator
                touch $dir/success/$file$s$c
				echo $curr/$dir/success/$file$s$c >> success_file_list_forpopulator
                cp ./$file$t $dir/$file$t$c
				echo $curr/$dir/$file$t$c >> file_list_forpopulator
                touch $dir/success/$file$t$c
				echo $curr/$dir/success/$file$t$c >> success_file_list_forpopulator
                c=`expr $c + 1`
        done
else
        echo "SPECIFIED DIRECTORY NOT FOUND"
fi
