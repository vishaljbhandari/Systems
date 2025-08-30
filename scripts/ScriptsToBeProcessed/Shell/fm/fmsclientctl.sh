#!/usr/bin/env bash

CURRENT_DIR=`dirname $0`
TMP_FILE_PATH=$COMMON_MOUNT_POINT/Client/tmp/start_servers.tmp
DASHBOARD_LOG=$COMMON_MOUNT_POINT/Client/log/dashboard.log
>$DASHBOARD_LOG
ALREADY_RUNNING="[ \x1b[38;32mAlready Running\x1b[38;0m ]"
STOPPED_MESSAGE="[     \x1b[38;32mStopped\x1b[38;0m     ]"
NOT_RUNNING="[   \x1b[38;31mNot Running\x1b[38;0m   ]"
RUNNING="[     \x1b[38;32mRunning\x1b[38;0m     ]"
FAILED="[     \x1b[38;31mFailed\x1b[38;0m      ]"
STARTED="[     \x1b[38;32mStarted\x1b[38;0m     ]"
RELOADED="[     \x1b[38;32mReloaded\x1b[38;0m    ]"

APACHE_PATH=`which apachectl 2>/dev/null` 
if [ "${APACHE_PATH}x" == "x" ]
then
	APACHE_ENABLED=0
else
	APACHE_ENABLED=1
	DIRNAME=`dirname $APACHE_PATH`
	APACHE_PID_FILE=${DIRNAME//\/bin/\/logs/httpd.pid}
fi

NOTIFICATIONMANAGER_PID_FILE=$CURRENT_DIR/log/notificationmanager.pid
QUERYMANAGER_PID_FILE=$CURRENT_DIR/log/querymanager.pid
NOTIFICATIONMANAGER_DIR=$CURRENT_DIR/../notificationmanager
QUERYMANAGER_DIR=$CURRENT_DIR/../querymanager
MEMCACHED_PID_FILE=$CURRENT_DIR/log/memcached.pid
JUGGERNAUT_PID_FILE=$CURRENT_DIR/log/juggernaut.pid
DASHBOARD_PID_FILE=$CURRENT_DIR/log/dashboard.pid
MEMCACHED_TYPE=`cat config/memcache.yml  | grep type | cut -d" " -f 2`
if [ $MEMCACHED_TYPE == "local" ];then
	MEMCACHE_RUN_MSG="[ \x1b[38;32mLocal Memcache Used\x1b[38;0m ]"
else
	MEMCACHE_RUN_MSG=$ALREADY_RUNNING
fi
MEMCACHED_PORT=`cat config/memcache.yml  | grep port | cut -d" " -f 2`

QUERYMANAGER_CONFIG=$QUERYMANAGER_DIR/config/query_manager.yml
QM_NEEDED=$(grep ':start_qm:' $QUERYMANAGER_CONFIG |tr -d ' '|cut -d':' -f3)
if [ "$QM_NEEDED" = "false" ];then
	QM_ENABLED=0
else
	QM_ENABLED=1
fi;

checkDashBoardRequirements() 
{
dashboard="dashboard.txt"
sqlplus /nolog << EOF > $dashboard 2>&1
connect ${DASHBOARD_DB_USER}/${DASHBOARD_DB_PASSWORD}

set feedback on ;
set serveroutput on ;
WHENEVER OSERROR  EXIT 6 ;
WHENEVER SQLERROR EXIT 5 ;


var exit_val number ;
var rol_count_create number ;
var rol_count_view number ;

declare
    resource_busy EXCEPTION ;
    pragma exception_init (resource_busy,-54) ;
begin
    :exit_val := 0 ;
    :rol_count_create := 0 ;
    :rol_count_view := 0 ;

    SELECT count(*) into :rol_count_create from role_tbl where rol_name = 'Create' ;
    SELECT count(*) into :rol_count_view from role_tbl where rol_name = 'View' ;

    if (:rol_count_create = 0 or :rol_count_view = 0)
    then
        :exit_val := 7 ;
    end if ;
exception
    when resource_busy then
        :exit_val := 99 ;
    when others then
        :exit_val := 6 ;
end ;
/
exit :exit_val ;
EOF
    if [ $? -ne 0 ]
    then
        IS_DASHBOARD_ENABLED=0
    fi
}
rm -rf $dashboard

if [ "${DASHBOARD_DB_USER}x" == "x" ]
then
	DASHBOARD_DB_USER='dashboard'
fi

if [ "${DASHBOARD_DB_PASSWORD}x" == "x" ]
then
	DASHBOARD_DB_PASSWORD='dashboard'
fi

if [ -f "$DASHBOARD_HOME/conf/server.xml" ]
then
	IS_DASHBOARD_ENABLED=1
	checkDashBoardRequirements
else
	IS_DASHBOARD_ENABLED=0
fi


wait_and_kill_pid()
{
	PROCESS_PID=$1
	PROCESS_TIMEOUT=$2
	if [ "x$PROCESS_TIMEOUT" == "x"  ]
	then
		PROCESS_TIMEOUT=10
	fi
	CURR_TIME=0
	kill -TERM $PROCESS_PID
	if [ "x`ps -u$LOGNAME | grep $PROCESS_PID`" == "x" ]
	then
		PROCESS_STATUS="0"
	else
		PROCESS_STATUS="1"
	fi

	while [ "$PROCESS_STATUS" = "1" -a $CURR_TIME -lt $PROCESS_TIMEOUT ]
	do
		sleep 1
		((CURR_TIME=$CURR_TIME+1))
	    if [ "x`ps -u$LOGNAME | grep $PROCESS_PID`" == "x" ]
		then
  			PROCESS_STATUS="0"
		else
			PROCESS_STATUS="1"
		fi
	done
	if [ "$PROCESS_STATUS" = "1" ]
	then
		kill -KILL $PROCESS_PID
	fi
}

get_dashboard_status()
{
	if [ -f "$DASHBOARD_PID_FILE" ]
	then
		DASHBOARD_PID=`cat $DASHBOARD_PID_FILE`
		if [ "`ps -u$LOGNAME | grep $DASHBOARD_PID`x" == "x" ]
		then
			DASHBOARD_STATUS="n"
		else
			DASHBOARD_STATUS="y"
		fi
	else
		DASHBOARD_STATUS="n"
	fi
}

start_dashboard()
{
  if [ $IS_DASHBOARD_ENABLED -eq 1 ]
  then
    put_starting_message "Starting Dashboard:"
    get_dashboard_status
    if [ $DASHBOARD_STATUS == "y" ]
    then
      ruby -e "puts \"$ALREADY_RUNNING\""
    else
      cd $DASHBOARD_HOME/bin
      java -jar -XX:PermSize=64m -XX:MaxPermSize=512m -Xms128m -Xmx1024m bootstrap.jar start > $OLDPWD/$DASHBOARD_LOG 2>&1 &

      log_file=$OLDPWD/$DASHBOARD_LOG
      dashboard_pid=$!
      cd $OLDPWD
      echo $dashboard_pid > $DASHBOARD_PID_FILE
      sleep 2
      get_dashboard_status
      if [ $DASHBOARD_STATUS == 'y' ]
      then
        status=`grep "INFO: Server startup" $log_file`
        while [ "x$status" == "x" ]
        do
          sleep 5
          status=`grep "INFO: Server startup" $log_file`
        done
        ruby -e "puts \"$STARTED\""
      else
        ruby -e "puts \"$FAILED\""
      fi
    fi
  fi
}

stop_dashboard() 
{
  if [ $IS_DASHBOARD_ENABLED -eq 1 ]
  then
    put_starting_message "Stopping Dashboard:"
    get_dashboard_status
    if [ $DASHBOARD_STATUS == 'n' ] 
    then
      ruby -e "puts \"$NOT_RUNNING\""
     else
      kill -TERM $DASHBOARD_PID >> $TMP_FILE_PATH 2>&1
      sleep 10
      get_dashboard_status
      if [ $DASHBOARD_STATUS == "y" ]
      then
      cd $DASHBOARD_HOME/bin		
        java -jar bootstrap.jar stop
      cd $OLDPWD 
      fi

      get_dashboard_status
      if [ $DASHBOARD_STATUS == "n" ]	
      then
        ruby -e "puts \"$STOPPED_MESSAGE\""
        rm -rf $DASHBOARD_PID_FILE
      else
        ruby -e "puts \"$FAILED\""
      fi
    fi
  fi
}

get_apache_status()
{
	if [ -f "$APACHE_PID_FILE" ]
	then
		APACHE_PID=`cat $APACHE_PID_FILE`
		if [ "`ps -u$LOGNAME | grep $APACHE_PID`x" == "x" ]
		then
			APACHE_STATUS="n"
		else
			APACHE_STATUS="y"
		fi
	else
		APACHE_STATUS="n"
	fi
}

get_juggernaut_status()
{
	if [ -f "$JUGGERNAUT_PID_FILE" ]
	then
		JUGGERNAUT_PID=`cat $JUGGERNAUT_PID_FILE`
		if [ "`ps -u$LOGNAME | grep $JUGGERNAUT_PID`x" == "x" ]
		then
			JUGGERNAUT_STATUS="n"
		else
			JUGGERNAUT_STATUS="y"
		fi
	else
		JUGGERNAUT_STATUS="n"
	fi
}

get_notificationmanager_status()
{
	if [ -f "$NOTIFICATIONMANAGER_PID_FILE" ]
	then
		
		NOTIFICATIONMANAGER_PID=`cat $NOTIFICATIONMANAGER_PID_FILE`
		if [ "`ps -u$LOGNAME | grep $NOTIFICATIONMANAGER_PID`x" == "x" ]
		then
			NOTIFICATIONMANAGER_STATUS="n"
		else
			NOTIFICATIONMANAGER_STATUS="y"
		fi
	else
		NOTIFICATIONMANAGER_STATUS="n"
	fi
}

get_querymanager_status()
{
	if [ -f "$QUERYMANAGER_PID_FILE" ]
	then
		
		QUERYMANAGER_PID=`cat $QUERYMANAGER_PID_FILE`
		if [ "`ps -u$LOGNAME | grep $QUERYMANAGER_PID`x" == "x" ]
		then
			QUERYMANAGER_STATUS="n"
		else
			QUERYMANAGER_STATUS="y"
		fi
	else
		QUERYMANAGER_STATUS="n"
	fi
}

get_memcached_status()
{
	if [ $MEMCACHED_TYPE = "remote" ];then
		if [ -f "$MEMCACHED_PID_FILE" ]
		then

			MEMCACHED_PID=`cat $MEMCACHED_PID_FILE`
			if [ "`ps -u$LOGNAME | grep $MEMCACHED_PID`x" == "x" ]
			then
				MEMCACHED_STATUS="n"
			else
				MEMCACHED_STATUS="y"
			fi
		else
			MEMCACHED_STATUS="n"
		fi
	else
		MEMCACHED_STATUS="y"
	fi
}

put_starting_message()
{
	MESSAGE=$1
	((LENGTH=61-${#MESSAGE}))
	printf "$MESSAGE"
	I=0
	while [ $I -lt $LENGTH ]
	do
		printf " "
		((I=$I+1))
	done
}

start_memcached()
{
	put_starting_message "Starting Memcached:"
	get_memcached_status
	if [ $MEMCACHED_STATUS == "y" ]
	then
		ruby -e "puts \"$MEMCACHE_RUN_MSG\""
	else
		memcached -p $MEMCACHED_PORT &
		memcache_pid=$!
		echo $memcache_pid > $MEMCACHED_PID_FILE
		sleep 2
		get_memcached_status
		if [ $MEMCACHED_STATUS == "y" ]	
		then
			ruby -e "puts \"$STARTED\""
		else
			ruby -e "puts \"$FAILED\""
		fi
	fi
}

stop_memcached()
{
	put_starting_message "Stopping Memcached:"
	get_memcached_status
	if [ $MEMCACHED_TYPE = "remote" ];then
		if [ $MEMCACHED_STATUS == "n" ]
		then
			ruby -e "puts \"$NOT_RUNNING\""
		else
			wait_and_kill_pid $MEMCACHED_PID

			get_memcached_status
			if [ $MEMCACHED_STATUS == "n" ]	
			then
				ruby -e "puts \"$STOPPED_MESSAGE\""
				rm -rf $MEMCACHED_PID_FILE
			else
				ruby -e "puts \"$FAILED\""
			fi
		fi
	else
		echo -e $MEMCACHE_RUN_MSG
	fi
}

start_notificationmanager()
{
	put_starting_message "Starting NotificationManager:"
	get_notificationmanager_status
	if [ $NOTIFICATIONMANAGER_STATUS == "y" ]
	then
		ruby -e "puts \"$ALREADY_RUNNING\""
	else
		$NOTIFICATIONMANAGER_DIR/server.rb >> $TMP_FILE_PATH 2>&1 &
		notificationmanager_pid=$!
		echo $notificationmanager_pid > $NOTIFICATIONMANAGER_PID_FILE
		sleep 2
		get_notificationmanager_status
		if [ $NOTIFICATIONMANAGER_STATUS == "y" ]	
		then
			ruby -e "puts \"$STARTED\""
		else
			ruby -e "puts \"$FAILED\""
		fi
	fi
}

stop_notificationmanager()
{
	put_starting_message "Stopping NotificationManager:"
	get_notificationmanager_status
	if [ $NOTIFICATIONMANAGER_STATUS == "n" ]
	then
		ruby -e "puts \"$NOT_RUNNING\""
	else
		wait_and_kill_pid $NOTIFICATIONMANAGER_PID

		get_notificationmanager_status
		if [ $NOTIFICATIONMANAGER_STATUS == "n" ]	
		then
			ruby -e "puts \"$STOPPED_MESSAGE\""
			rm -rf $NOTIFICATIONMANAGER_PID_FILE
		else
			ruby -e "puts \"$FAILED\""
		fi
	fi
}

start_querymanager()
{
	if [ $QM_ENABLED -eq 1 ];then
		put_starting_message "Starting Querymanager:"
		get_querymanager_status
		if [ $QUERYMANAGER_STATUS == "y" ]
		then
			ruby -e "puts \"$ALREADY_RUNNING\""
		else
			ruby $QUERYMANAGER_DIR/server.rb >> $TMP_FILE_PATH 2>&1 &
			querymanager_pid=$!
			echo $querymanager_pid > $QUERYMANAGER_PID_FILE
			sleep 2
			get_querymanager_status
			if [ $QUERYMANAGER_STATUS == "y" ]	
			then
				ruby -e "puts \"$STARTED\""
			else
				ruby -e "puts \"$FAILED\""
			fi
		fi
	fi
}

stop_querymanager()
{
	if [ $QM_ENABLED -eq 1 ];then
		put_starting_message "Stopping Querymanager:"
		get_querymanager_status
		if [ $QUERYMANAGER_STATUS == "n" ]
		then
			ruby -e "puts \"$NOT_RUNNING\""
		else
			wait_and_kill_pid $QUERYMANAGER_PID

			get_querymanager_status
			if [ $QUERYMANAGER_STATUS == "n" ]	
			then
				ruby -e "puts \"$STOPPED_MESSAGE\""
				rm -rf $QUERYMANAGER_PID_FILE
			else
				ruby -e "puts \"$FAILED\""
			fi
		fi
	fi
}

start_apache()
{
  if [ $APACHE_ENABLED -eq 1 ]
  then
    put_starting_message "Starting Apache:"
    get_apache_status
    if [ $APACHE_STATUS == "y" ]
    then
      ruby -e "puts \"$ALREADY_RUNNING\""
    else
      apachectl start  >> $TMP_FILE_PATH 2>&1
      sleep 2
      get_apache_status
      if [ $APACHE_STATUS == "y" ]	
      then
        ruby -e "puts \"$STARTED\""
      else
        ruby -e "puts \"$FAILED\""
      fi
    fi
  fi
}

stop_apache()
{
  if [ $APACHE_ENABLED -eq 1 ]
  then
    put_starting_message "Stopping Apache:"
    get_apache_status
    if [ $APACHE_STATUS == "n" ]
    then
      ruby -e "puts \"$NOT_RUNNING\""
    else
      apachectl stop  >> $TMP_FILE_PATH 2>&1
      sleep 2
      get_apache_status
      if [ $APACHE_STATUS == "n" ]	
      then
        ruby -e "puts \"$STOPPED_MESSAGE\""
      else
        ruby -e "puts \"$FAILED\""
      fi
    fi
	fi
}

start_juggernaut()
{
	put_starting_message "Starting Juggernaut:"
	get_juggernaut_status
	if [ $JUGGERNAUT_STATUS == "y" ]
	then
		ruby -e "puts \"$ALREADY_RUNNING\""
	else
		juggernaut -c $CURRENT_DIR/juggernaut.yml -l $COMMON_MOUNT_POINT/Client/log/juggernaut.log -P $CURRENT_DIR/log/juggernaut.pid -e -d  >> $TMP_FILE_PATH 2>&1
		sleep 2
		get_juggernaut_status
		if [ $JUGGERNAUT_STATUS == "y" ]	
		then
			ruby -e "puts \"$STARTED\""
		else
			ruby -e "puts \"$FAILED\""
		fi
	fi
}

stop_juggernaut()
{
	put_starting_message "Stopping Juggernaut:"
	get_juggernaut_status
	if [ $JUGGERNAUT_STATUS == "n" ]
	then
		ruby -e "puts \"$NOT_RUNNING\""
	else
		wait_and_kill_pid $JUGGERNAUT_PID
		get_juggernaut_status
		if [ $JUGGERNAUT_STATUS == "n" ]	
		then
			ruby -e "puts \"$STOPPED_MESSAGE\""
			rm -rf $JUGGERNAUT_PID_FILE
		else
			ruby -e "puts \"$FAILED\""
		fi
	fi
}

print_status()
{
	for module in $@
	do
		MODULE=`echo ${module} | perl -p -e 's/([A-Z])/lc($1)/ge'`
		put_starting_message $module:
		get_${MODULE}_status
		cmd=`echo ${module} | perl -p -e 's/([a-z])/uc($1)/ge'`_STATUS
		eval "STATUS=\$${cmd}"
		if [ "$STATUS" == "y" ]
		then
			if [ $MODULE = "memcached" ];then
				echo -e "$MEMCACHE_RUN_MSG"
			else
				ruby -e "puts \"$RUNNING\""
			fi;
		else
			ruby -e "puts \"$NOT_RUNNING\""
		fi
	done
}

start_without_dashboard()
{
>$TMP_FILE_PATH
	start_juggernaut
	start_memcached
	start_querymanager
	start_notificationmanager
#	start_apache
}

start_all()
{
>$TMP_FILE_PATH
	start_juggernaut
	start_memcached
	start_notificationmanager
	start_querymanager
	start_dashboard
	start_apache
}

reload_notificationmanager()
{
	get_notificationmanager_status
	if [ "$NOTIFICATIONMANAGER_STATUS" == "y" ]
	then
		put_starting_message "Reloading NotificationManager:"
		kill -HUP `cat $NOTIFICATIONMANAGER_PID_FILE`
		ruby -e "puts \"$RELOADED\""	
	else
		print_status notificationmanager
	fi
}

stop_all()
{
	stop_apache
	stop_querymanager
	stop_notificationmanager
	stop_memcached
	stop_juggernaut
	stop_dashboard
}
ARGV="$@"
if [ "x$ARGV" == "x" ]
then
#--------------------------------------------------------------------------------------------
trap 'get_window_size' WINCH                    # trap when a user has resized the window

_UNDERLINE_ON=`tput smul`                       # turn on underline
_UNDERLINE_OFF=`tput rmul`                      # turn off underline
_BOLD=`tput bold`
_NORM=`tput sgr0`
_OPTION_ROW=2
_INIT_RESULT_ROW=`expr ${_OPTION_ROW} + 18`
_ERROR_ROW=`expr ${_OPTION_ROW} + 1`
_OPTION_PREV_ROW=`expr ${_OPTION_ROW} - 1`
get_window_size() {
  _WINDOW_X=`tput lines`
  _WINDOW_Y=`tput cols`

   _WINDOW_Y=`expr ${_WINDOW_Y} - 10`
  _FULL_SPACES=`echo ""|awk '
  {
    _SPACES = '${_WINDOW_Y}'
    while (_SPACES-- > 0) printf (" ")
  }'`
  _FULL_UNDERLINE=`echo "${_UNDERLINE_ON}${_FULL_SPACES}${_UNDERLINE_OFF}"`

  unset _FULL_SPACES
}


clear_results() {
    x_axis=`tput cols`
    tput civis
    tput cup ${_ERROR_ROW} 0
    ruby -e "puts ' ' * $x_axis"

    result_row=${_INIT_RESULT_ROW}

    count=30
    while [ $count -ne 0 ] 
    do
      let count--
      let result_row++
      tput cup $result_row 0
      ruby -e "puts ' ' * $x_axis"
    done

    result_row=`expr ${_INIT_RESULT_ROW} + 1`
    tput cup ${result_row} 0
    tput cnorm
}

show_menu() {
  while [[ -z ${_ANS} ]]
  do
    tput civis
    tput cup 0 0 
    get_window_size
    tput bold

    cat <<- EOF
		${_FULL_UNDERLINE}
`tput cup ${_OPTION_PREV_ROW} 16`
		Enter Option =>                                                       
    $_NORM
		${_FULL_UNDERLINE}

    ${_BOLD}A${_NORM}) Start All
    ${_BOLD}B${_NORM}) Stop All
    ${_BOLD}C${_NORM}) Status
    ${_BOLD}D${_NORM}) Restart
    ${_BOLD}E${_NORM}) Stop Dashboard
    ${_BOLD}F${_NORM}) Start Dashboard
    ${_BOLD}G${_NORM}) Stop Apache
    ${_BOLD}H${_NORM}) Start Apache
    ${_BOLD}I${_NORM}) start_without_dashboard
    ${_BOLD}J${_NORM}) Clear Cache

    ${_BOLD}Q${_NORM}) Exit
		${_FULL_UNDERLINE}
    Also you can use : $0 {start|stop|restart|status|clearcache|start_without_dashboard|start_notificationmanager}
    Ex: $0 status
		${_FULL_UNDERLINE}
	EOF

    tput rc
    tput cnorm
    read _ANS
    
    clear_results

    case ${_ANS} in
      [Aa]) start_all ;;
      [Bb]) stop_all ;;
      [Cc])  
      		if [ $IS_DASHBOARD_ENABLED -eq 1 ]
          then
            DASHBOARD_PARAM=Dashboard
          fi
          
          if [ $APACHE_ENABLED -eq 1 ]
          then
            APACHE_PARAM=Apache
          fi
          print_status Juggernaut Memcached Querymanager Notificationmanager $APACHE_PARAM $DASHBOARD_PARAM
        ;;
      [Dd])  
          stop_all
          start_all
        ;;
      [Ee]) stop_dashboard ;;
      [Ff]) start_dashboard ;;
      [Gg]) stop_apache ;;
      [Hh]) start_apache ;;
      [Ii]) start_without_dashboard ;;
      [Jj]) rake tmp:cache:clear ;;
      [Qq]) tput clear; exit;;
         *)
             tput cup ${_ERROR_ROW} 0
             echo -e "    Invalid Selection: ${_ANS}\c"
             tput cup ${_OPTION_ROW} 16
             ;;
    esac
    unset _ANS
  done
}

tput sgr0
tput reset
tput cup ${_OPTION_ROW} 16
tput sc

[[ -n ${_ANS} ]] && unset _ANS
show_menu
#--------------------------------------------------------------------------------------------
else
case $ARGV in
	start)
		start_all
	;;	
	stop)
		stop_all
	;;	
	restart)
	    stop_all
    	start_all
	;;
	reload_notificationmanager)
		reload_notificationmanager
	;;
	start_without_dashboard)
		start_without_dashboard
	;;
	clearcache)
		rake tmp:cache:clear
	;;
		
	status)
		if [ $IS_DASHBOARD_ENABLED -eq 1 ]
		then
			DASHBOARD_PARAM=Dashboard
		fi
		
		if [ $APACHE_ENABLED -eq 1 ]
		then
			APACHE_PARAM=Apache
		fi
		if [ $QM_ENABLED -eq 1 ];then
			QM_PARAM=Querymanager
		fi
		print_status Juggernaut Memcached $QM_PARAM Notificationmanager $APACHE_PARAM $DASHBOARD_PARAM
	;;
esac	

fi


