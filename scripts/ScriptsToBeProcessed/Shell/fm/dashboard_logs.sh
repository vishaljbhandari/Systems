#! /usr/bin/bash
#################################################################################
#  Copyright (c) Subex Limited. All rights reserved.                            #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Azure Limited. The program (s) may be used and/or copied with the written    #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

#########################################################################################################
# USAGE: scriptlauncher dashboard_logs.sh
#########################################################################################################

. rangerenv.sh

# THE 'top' and 'sar' command path has to be set either in Bash properly, or add in the below line
export PATH=$PATH:/opt/sfw/bin/

#########################################################################################################
# DDL:
#-----
#CREATE TABLE CPU_LOG (ID NUMBER, TIME DATE, usr number, sys number, wio number, idle number, machine_name varchar2(32)) ;
#CREATE SEQUENCE CPU_LOG_ID INCREMENT BY 1 NOMAXVALUE MINVALUE 1 NOCYCLE CACHE 20 ORDER ;

#CREATE TABLE RAM_LOG (ID NUMBER, TIME DATE, TOTAL varchar2(16), free varchar2(16), machine_name varchar2(32)) ;
#CREATE SEQUENCE RAM_LOG_ID INCREMENT BY 1 NOMAXVALUE MINVALUE 1 NOCYCLE CACHE 20 ORDER ;

#CREATE TABLE DISK_LOG (ID NUMBER, TIME DATE, file_system varchar2(256), total number, used number, free number, capacity number, mounted_on varchar2(256), machine_name varchar2(32)) ;
#CREATE SEQUENCE DISK_LOG_ID INCREMENT BY 1 NOMAXVALUE MINVALUE 1 NOCYCLE CACHE 20 ORDER ;
#########################################################################################################
get_ip() {
    os=`uname`
    ip=""
    if [ "$os" == "Linux" ]
    then
        ip=`/sbin/ifconfig  | grep 'inet addr:'| grep -v '127.0.0.1' | cut -d: -f2 | awk '{ print $1}'`
    else
        if [ "$os" == "SunOS" ]
        then
            ip=`ifconfig -a | grep inet | grep -v '127.0.0.1' | head -1 | awk '{ print $2} '`
        fi
    fi
    echo $ip
}

#########################################################################################################
convert_into_bytes() 
{
	val=$1
	if [ `echo $val | grep -i 'T$'` ] || [ `echo $val | grep -i 'TB$'` ]
	then
		val=`echo $val | sed 's/T//I' | sed 's/TB//I'`
		val=`echo $val | awk '{print $1 * 1073741824 * 1024}'`
	else
		if [ `echo $val | grep 'G$'` ] || [ `echo $val | grep 'GB$'` ]
		then
			val=`echo $val | sed 's/G//' | sed 's/GB//I'`
			val=`echo $val | awk '{print $1 * 1073741824}'`
		else
			if [ `echo $val | grep 'M$'` ] || [ `echo $val | grep 'MB$'` ]
			then
				val=`echo $val | sed 's/M//I' | sed 's/MB//I' ` 
				val=`echo $val | awk '{print $1 * 1048576}'`
			else
				if [ `echo $val | grep -i 'KB$'` ] || [ `echo $val | grep -i 'k$'` ]
				then
					val=`echo $val | sed 's/KB//I' | sed 's/k//' | sed 's/K//'` 
					val=`echo $val | awk '{print $1 * 1024}'`
				fi
			fi
		fi
	fi
	echo $val
}

#########################################################################################################
execute_query()
{
  query=$*
  $SQL_COMMAND -s /nolog << EOF
    CONNECT_TO_SQL
    set feedback off;
    whenever sqlerror exit 5 ;
    $query
    commit;
    quit;
EOF
result=$?
  if [ $result -ne 0 ]
  then
     echo "ERROR: There was some error while executing query '$query'"
     exit 1
  fi
}
#########################################################################################################
# Global Variables
which_OS=`uname`
sar_exists=`which sar | grep "/sar" | wc -l`
top_exists=`which top | grep "/top" | wc -l`
top_path=`which top`

#echo "sar_exists = $sar_exists"
#echo "top_exists = $top_exists"
ipaddress=`get_ip`

#########################################################################################################
# Function: cpu_info_using_top
# Description: Finds the Cpu status from the 'top' command and stores User, System, IO Wait and IDLE time
#########################################################################################################
cpu_info_using_top()
{
  if [ $top_exists -eq 1 ]
  then
    echo "Generating CPU Info"
    if [ $which_OS == 'SunOS' ]
    then
      #1   2       3     4      5    6      7    8        9    10       11   12
      #CPU states: 98.3% idle,  0.6% user,  1.1% kernel,  0.0% iowait,  0.0% swap
      #avg=`$top_path -b -d5 | grep "CPU states:" | tail -1|tr -s ' '|awk '{print $5, $7, $3, $9}'`
		avg=`$top_path -b -d5 | grep 'CPU states' | tail -1 | sed 's/ *//g' | sed 's/.*://g' | gawk -F {print $2","$3","$1","$4}`
    else
      if [ $which_OS == 'Linux' ]
      then
        #1        2    3    4    5    6    7   8     9    10   11   12   13   14   15
        #Cpu(s):  0.2% us,  0.2% sy,  0.0% ni, 99.7% id,  0.0% wa,  0.0% hi,  0.0% si
        #avg=`top -b -n5|head|grep Cpu|tr -s ' '|awk '{print $2, $4, $8, $10}'`
        avg=`top -b -n5 | head | grep 'Cpu' | sed 's/ *//g' | sed 's/.*://g' | awk -F',' '{print $1","$2","$4","$5}'`
      else
        echo "OS not supported, contact developers!!!"
        exit -1
      fi
    fi

    cnt=1
	for i in $(echo $avg | tr "," "\n")
	do 
		cpu[cnt]=`echo $i | sed 's/%.*//g'`
		let cnt++ 
	done

    USR=${cpu[1]}; SYS=${cpu[2]}; WIO=${cpu[4]}; IDLE=${cpu[3]};

    query="insert into CPU_LOG values (CPU_LOG_ID.nextval, sysdate, $USR, $SYS, $WIO, $IDLE, '$ipaddress') ;"
    execute_query "$query"

  else
    echo "'top' command not found, please contact adminstrator"
    exit -1 ;
  fi
}

#########################################################################################################
# Function: disk_info_using_df
# Description: Finds the Disk usage from the 'df' command
#########################################################################################################
processLineForDiskInfo(){
  line="$@" # get all args

  file_system=$(echo $line | awk '{ print $1 }')
  total=$(echo $line | awk '{ print $2 }')
  used=$(echo $line | awk '{ print $3 }')
  free=$(echo $line | awk '{ print $4}')
  capacity=$(echo $line | awk '{ print $5 }' |sed 's/\%//g')
  mounted_on=$(echo $line | awk '{ print $6 }')
  sysdate=$(echo $line | awk '{ print $7 }')
  systime=$(echo $line | awk '{ print $8 }')

  total=`convert_into_bytes $total`
  free=`convert_into_bytes $free`
  used=`convert_into_bytes $used`

  values="DISK_LOG_ID.nextval, to_date('$sysdate $systime', 'dd-mm-yyyy HH24:mi:ss'), '$file_system', $total, $used, $free, $capacity, '$mounted_on', '$ipaddress'"

  query="insert into disk_log values($values) ;"
  execute_query $query ;
}

disk_info_using_df()
{
  tmp_file="disk_info.log"
  local_time=`date +"%d-%m-%Y %H:%M:%S"`

  echo "Generating Disk Info"
  # Get the Disk info
  df -khP > $tmp_file

  if [ ! -f $tmp_file ]
  then
    echo "ERROR: Problem in checking the Disk space, contact administrator"
    exit -1
  fi

  # Set loop separator to end of line
  line_cnt=0
  #BAKIFS=$IFS
  #ifs=$(echo -en "\n\b")
  exec 3<&0
  exec 0<$tmp_file
  while read line
  do
    if [ $line_cnt -ne 0 ]
    then
      if [ $which_OS == 'SunOS' -o $which_OS == 'Linux' ]
      then
       # use $line variable to process line in processLine() function
       processLineForDiskInfo "$line $local_time"
      else
        echo "OS not supported, contact developers!!!"
        exit -1
      fi
    fi
    let line_cnt++
  done
  exec 0<&3
  # restore $IFS which was used to determine what the field separators are
  #IFS=$BAKIFS
  rm -rf $tmp_file
}

#########################################################################################################
# Function: ram_info_using_top
# Description: Finds the Ram usage from the 'top' command and stores Total and Free memory
#########################################################################################################
ram_info_using_top()
{
  if [ $top_exists -eq 1 ]
  then
    echo "Generating RAM Info"
    if [ $which_OS == 'SunOS' ]
    then
      #1       2     3     4    5     6     7    8  9    10  11   12
      #Memory: 8192M real, 249M free, 8658M swap in use, 14G swap free
      #avg=`top -b -n 2| grep "Memory:" | tail -1|tr -s ' '|awk '{print $2, $4}'`

      #1       2     3  4    5   6    7    8   9     10  11   12  -- In Dubai Machine
      #Memory: 24G phys mem, 13G free mem, 32G swap, 31G free swap
      avg=`top -b -n 2| grep "Memory:" | tail -1|tr -s ' '|awk '{print $2, $5}'`
    else
      if [ $which_OS == 'Linux' ]
      then
        #1    2        3      4        5     6      7     8      9
        #Mem: 1026428k total, 1004672k used, 21756k free, 43480k buffers
        avg=`top -b -n2|head|grep "Mem:" |tr -s ' ' | awk '{print $2, $6}'`
      else
        echo "OS not supported, contact developers!!!"
        exit -1
      fi
    fi

    cnt=1
    for i in $avg; do
    cpu[$cnt]=`echo $i` #|sed 's/\k//g'`
    let cnt++
    done

    TOTAL=${cpu[1]}; FREE=${cpu[2]};
    TOTAL=`convert_into_bytes $TOTAL`
    FREE=`convert_into_bytes $FREE`
    
    query="insert into RAM_LOG values (RAM_LOG_ID.nextval, sysdate, '$TOTAL', '$FREE', '$ipaddress') ;"
    #echo $query ;
    execute_query $query

  else
    echo "'top' command not found, please contact adminstrator"
    exit -1 ;
  fi
}

#########################################################################################################
# SOLARIS: sar output:Average %usr %sys %wio %idle
# LINUX: sar output:  Average CPU %user %nice %system %iowait %steal %idle
#########################################################################################################
cpu_info_using_sar()
{
  if [ $sar_exists -eq 1 ]
  then
    echo "Generating CPU info using sar"

    if [ $which_OS == 'SunOS' -o $which_OS == 'Solaris' ]
    then
      avg=`sar -u 1 5 | tail -1 | awk '{print $2,  $3,  $4,  $5}'`
    else
      if [ $which_OS == 'Linux' -o $which_OS == 'Unix' ]
      then
        avg=`sar -u 1 10 | tail -1 | awk '{print $3,  $5,  $6,  $8}'`
      else
        echo "OS not supported, contact developers!!!"
        exit -1
      fi
    fi

cnt=1
for i in $avg; do
cpu[$cnt]=$i
let cnt++
done

    USR=${cpu[1]}
    SYS=${cpu[2]}
    WIO=${cpu[3]}
    IDLE=${cpu[4]}

        #echo "$USR, $SYS, $WIO, $IDLE"
        query="insert into CPU_LOG values (CPU_LOG_ID.nextval, sysdate, $USR, $SYS, $WIO, $IDLE, '$ipaddress') ;"
        #echo $query ;
        execute_query "$query"


  else
    echo "'sar' command not found, please contact adminstrator"
    exit -1 ;
  fi
}


#########################################################################################################
# Function: main
# Description: Main functions which call other functions to populate required data
#########################################################################################################
main()
{
  echo "Deleting Old Records less than 30 min in RAM_LOG, CPU_LOG and DISK_LOG tables"
  execute_query "DELETE DISK_LOG WHERE TIME < sysdate - 30/(24*60) ;"
  execute_query "DELETE RAM_LOG WHERE TIME < sysdate - 30/(24*60) ;"
  execute_query "DELETE CPU_LOG WHERE TIME < sysdate - 30/(24*60) ;"

  echo "Generating $which_OS info at `date`"
  if [ $which_OS == 'SunOS' ]
  then
    cpu_info_using_sar
  else
    if [ $which_OS == 'Linux' ]
    then
      cpu_info_using_top
    fi
  fi
  if [ $which_OS == 'SunOS' -o $which_OS == 'Linux' ]
  then
    ram_info_using_top
    disk_info_using_df
  fi
 echo "Generated OS info at `date`"

  exit 0
}

# Call Main function to generate the informations
main
exit 0
