#!/bin/bash
echo "Enter program manager conf file path example--  /home/user/NIKIRAROOT/sbin/programmanager.conf.tcp "
read pgm_conf_file
if [ ! -f $pgm_conf_file ]
then
   echo "program manager file does not exists!!...";
   exit 1;
fi;
echo ""
cat $pgm_conf_file | tr -s " " | grep -v ^$  | grep -v ^#  | awk '{for (i=1; i<= NF ;i++) print $i"\n"}'| grep -v ^$ | grep -i TCP | awk -F":" '{print $NF}' | sed -e 's/"//g' |sort -u  > PMPorts

#start_port = 33131
#end_port = 44191
echo "Enter the available port range"
echo "Starting port"
read start_port
echo "Ending port"
read end_port
echo ""

if [ $start_port -gt $end_port ]
then
	echo "start port is greater than end port value, swapping the values...! "; 
	tmp=$start_port
	start_port=$end_port
	end_port=$tmp

fi;

NoofPorts=`awk 'END{print NR}' PMPorts`
echo "NoofPorts = $NoofPorts"

j=1
rm -f CanBeReplaced.txt
for ((i=$start_port ; i < $end_port ;i++))
do
    netstat -a | grep $i > /dev/null
    retcode=$?
    if test $retcode -ne 0
    then
        PMPort=`awk -v Rec=$j '{if (NR == Rec) print $0}' PMPorts`
        echo $PMPort-$i >>CanBeReplaced.txt
        let j=$j+1
    fi
    if test $j -eq $NoofPorts
    then
        cp $pgm_conf_file new.conf
        echo "#!/bin/bash" > GeneratePMConf.sh
        for rec in `cat CanBeReplaced.txt`
        do
            NewPort=`echo $rec | cut -d "-" -f2`
            OldPort=`echo $rec | cut -d "-" -f1`
            echo "perl -pi -e 's#:$OldPort#:$NewPort#g' new.conf"  >> GeneratePMConf.sh
        done
        chmod 777 GeneratePMConf.sh
        bash GeneratePMConf.sh
        #AlarmGenPort=`grep "ALARMGENERATOR = alarmgenerator -r " new.conf | tr -s " " | cut -d ":" -f3|cut -d " " -f1`
        #SmtPort=`grep "SMARTPATTERNPROCESSOR = smartpatternprocessor -r " new.conf | tr -s " " | cut -d ":" -f3|cut -d " " -f1`
        exit 1
    fi
done

