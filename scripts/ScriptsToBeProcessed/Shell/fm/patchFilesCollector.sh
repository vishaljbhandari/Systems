#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "Invalid arguments.."
	echo "Usage: $0 [FMSCLIENT|FMSROOT|FMSTOOLS]"
	exit 1
fi
while getopts m:h OPTION
do
        case ${OPTION} in
                m) ModuleName="${OPTARG}" ;;
                h) Help;exit 0;;
                \?) Help;exit 2;;
        esac
done

#echo "Module=$ModuleName"
if [ "$ModuleName" != "FMSCLIENT" -a "$ModuleName" != "FMSROOT" -a "$ModuleName" != "FMSTOOLS" ]
then
	echo "Invalid Module Name"
	echo "Valied Module Names are : FMSCLIENT FMSROOT FMSTOOLS"
	exit 0;
fi
#moduelDir=$(eval "echo \${$ModuleName}")
#echo "moduelDir=$moduelDir"
cd $(eval "echo \${$ModuleName}")


all_files=`cat ~/NBIT/PatchFilesWithIngore.conf|grep -v "^#"`
l1=`echo $all_files|tr " " "\n"|grep -n "IGNORE_LIST={"|cut -d':' -f1`
#echo $l1
l2=`echo $all_files|tr " " "\n"|grep -n "}"|cut -d':' -f1`
l2=`expr $l2 - 1`
#echo $l2
l3=`expr $l2 - $l1`
ignore_files=`echo $all_files|tr " " "\n"|head -$l2|tail -$l3`
#echo "$ignore_files=$ignore_files"
#ignore=`echo $ignore_files |tr " " "|"`
#echo $ignore


l3=`expr $l1 - 1`
final_files1=`echo $all_files |tr " " "\n"|head -$l3`

l4=`echo $all_files |tr " " "\n"|wc -l`
#echo $l4
l5=`expr $l4 - $l2 - 1`
#echo $l5
final_files2=`echo $all_files |tr " " "\n"|tail -$l5`
final_files1=`echo $all_files |tr " " "\n"|head -$l3`
final_files="$final_files1 $final_files2"
#echo $final_files

tar_files=""
for file in $final_files
do 
   files=""
   if [ -d $file ] 
   then 
        #echo "dir=$file"
        for i in `find test1`
        do 
            if [ ! -d $i ]
            then 
                files="$files $i"
            fi
        done
   else 
	if [ -f $file ]
        then
                #echo "file=$file"
                files=$file
        else
                echo "Error $file not exists"
        fi
   fi
   #echo "files=$files"
   filtered_files=`echo $files|tr " " "\n"|egrep -v "$ignore_files"|tr "\n" " "`
   #echo "filtered_files=$filtered_files"
   tar_files="$tar_files $filtered_files"
   #echo "$tar_files"
done
#echo "tar_files=$tar_files"
echo $tar_files|tr " " "\n" >~/NBIT/PatchFiles.conf


