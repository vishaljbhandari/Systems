#! /bin/bash
Initialize()
{  
		LOG_BEGIN='-------------------------------------- LOG STARTS ------------------------------------------------'
		LOG_END='-------------------------------------- LOG ENDS   ------------------------------------------------'
		count=2
		PREFIX=`cat $CONFIG_FILE | grep "SWITCH PREFIX" | cut -d '=' -f2 | sed 's:,: :g'`
		TEMP_DIR=`cat $CONFIG_FILE | grep "TEMP DIR" |  cut -d '=' -f2  | sed 's: :rm_spa_dirname:g' | sed 's:,: :g'`
}
DeleteLogsFolder()
{		
        EXTN=`cat $CONFIG_FILE | grep -w "DIR$cnt" |  cut -d ':' -f4 | cut -d '|' -f1`
        ACTION=`cat $CONFIG_FILE | grep -w "DIR$cnt" |  cut -d ':' -f5 | cut -d '|' -f1`
		DIR_PREF=`cat $CONFIG_FILE | grep -w "DIR$cnt" | cut -d ':' -f6`
		cd "$dir"
		if [ $DIR_PREF = 'N' ]
        then
            summary=summary.log
		else	
        	DIR_PRE="`basename $dir`"
			REJ=summary.log
			summary=$DIR_PRE$REJ
        fi
		
		
        for file_name in `ls -rt *.$EXTN  2> /dev/null `
        do
            file=$file_name
		    FILE_PREF=`echo $file_name | cut -d '.' -f1`
                    flag=1
                    for temp_prefix in $PREFIX
                    do
                        if [ $temp_prefix = $FILE_PREF ] && [ $DIR_PREF = 'N' ]
                        then
                            echo $LOG_BEGIN >> $COMMON_MOUNT_POINT/LOG/SummaryLog/$FILE_PREF$summary
                            echo $file_name >> $COMMON_MOUNT_POINT/LOG/SummaryLog/$FILE_PREF$summary
                            echo "------------" >> $COMMON_MOUNT_POINT/LOG/SummaryLog/$FILE_PREF$summary
                            cat "$dir/$file_name"  >> $COMMON_MOUNT_POINT/LOG/SummaryLog/$FILE_PREF$summary
                        	echo $LOG_END >> $COMMON_MOUNT_POINT/LOG/SummaryLog/$FILE_PREF$summary
							file=`echo $file_name | sed "s:$FILE_PREF.: :"`
                            flag=0
                        fi
                    done
                    if [ $flag = 1 ]
                    then
							echo $LOG_BEGIN >> $COMMON_MOUNT_POINT/LOG/SummaryLog/$summary
                            echo $file_name >> $COMMON_MOUNT_POINT/LOG/SummaryLog/$summary
                            echo "------------" >> $COMMON_MOUNT_POINT/LOG/SummaryLog/$summary
                            cat "$dir/$file_name"  >> $COMMON_MOUNT_POINT/LOG/SummaryLog/$summary
                        	echo $LOG_END >> $COMMON_MOUNT_POINT/LOG/SummaryLog/$summary
                    fi
                    rm -f "$dir/$file_name"
					if [ $ACTION = 'NA' ]
					then
							for TMP_DIR in $TEMP_DIR
							do
								TMP_DIR=`echo $TMP_DIR | sed 's:rm_spa_dirname: :g'`
								if [ -a "$TMP_DIR" ]
								then
									EXTN=`echo $EXTN | tr -d " "`
									file=`echo $file | sed "s:.$EXTN: :"`
									file=`echo $file | tr -d " "`
									rm -f "$TMP_DIR/$file" 
								else
									echo "DIR $TMP_DIR Not Exists" >> $COMMON_MOUNT_POINT/LOG/SummaryLog/cleanup.log
								fi
							done
					fi
        done
}
DeleteFilesUsingTimeConstraint()
{
		EXTN=`cat $CONFIG_FILE | grep -w "DIR$cnt" |  cut -d ':' -f4 | cut -d '|' -f1`
		if [ $EXTN = 'ALL' ]
		then
				find "$dir" -name '*' -atime $NO_OF_DAYS -exec rm {} \; 2> /dev/null
		else
				for((i=1; i <= $count; i++))
				do
						count=`expr $count + 1`
						EACH_EXTN=`echo $EXTN | cut -d ',' -f$i`
						if [ x$EACH_EXTN = 'x' ]
						then
								break
						fi
						find "$dir" -name "*.$EACH_EXTN" -atime $NO_OF_DAYS -exec rm {} \; 2> /dev/null
				done
		fi
}
ExecuteFunctionality()
{ 
		cnt=0
		while [ 1 ]	
		do	
					cnt=`expr $cnt + 1`
					dir=`cat $CONFIG_FILE | grep -w  "DIR$cnt" | cut -d ':' -f2 |cut -d ',' -f1`
							if [ "x$dir" = 'x' ]
							then
								break
							fi
					if [ -d "$dir" ]
					then
							NO_OF_DAYS=`cat $CONFIG_FILE | grep -w "DIR$cnt" | cut -d ':' -f3 | cut -d '|' -f1`
							if [ $NO_OF_DAYS = 'NA' ]
							then
									DeleteLogsFolder
							else
									DeleteFilesUsingTimeConstraint
							fi
					else
							echo "$dir Not FOUND" >> $COMMON_MOUNT_POINT/LOG/SummaryLog/cleanup.log
					fi
		done
}
main ()
{	
		. rangerenv.sh
		CONFIG_FILE=$COMMON_MOUNT_POINT/bin/directriescleanup.config
		mkdir -p $COMMON_MOUNT_POINT/LOG/SummaryLog
		Initialize
		while [ 1 ]
		do
		if [ -f $CONFIG_FILE ]
		then	
				ExecuteFunctionality
		else
				echo "Configuration File Not Found" >> $COMMON_MOUNT_POINT/LOG/SummaryLog/cleanup.log
		fi
		done
	
}
main $*



