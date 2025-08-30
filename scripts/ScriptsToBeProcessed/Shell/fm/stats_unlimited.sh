on_Exit()
{
        echo "STATS_UNLIMITED Exiting......"
	exit
}

on_SIGHUP()
{
        echo "STATS_UNLIMITED Exiting SIGHUP......"
	exit
}

trap on_Exit    EXIT
trap on_Exit    SIGINT
trap on_SIGHUP  SIGHUP


sarreport()
{
    sarfile=sar`date|tr -s " " | sed "s/[ :]/-/g"`
    sar -o "$sardir/$sarfile" 1 6 >/dev/null &
}

iostatreport()
{
    iofile=iostat`date|tr -s " " | sed "s/[ :]/-/g"`
    iostat -T d -xn 2 3  > "$iodir/$iofile" 2>&1 &
}

vmreport()
{
    vmfile=vmstats`date|tr -s " " | sed "s/[ :]/-/g"`
    vmstat -p 2 3 > "$vmdir/$vmfile" 2>&1  &
}

mpstatreport()
{
	mpstatfile=mpstat`date|tr -s " " | sed "s/[ :]/-/g"`
	top > "$mpstat/$mpstatfile" 2>&1  &
}

directory()
{
   	suffix=NV7_PTT_`date|tr -s " " | sed "s/[ :]/-/g"`
	sardir="/storage6/Statspack/$suffix/SAR"
	iodir="/storage6/Statspack/$suffix/IOSTAT"
	awrdir="/storage6/Statspack/$suffix/AWR"
	vmdir="/storage6/Statspack/$suffix/VMS"
	mpstat="/storage6/Statspack/$suffix/MPSTAT"
	mkdir -p $sardir
	mkdir -p $iodir
	ln -s /storage6/Statspack/extract-io.sh $iodir/extract-io.sh 
	mkdir -p $awrdir
	mkdir -p $vmdir
	mkdir -p $mpstat
	ln /storage6/Statspack/gatherDBStats.sh $awrdir/gatherDBStats.sh
	ln /storage6/Statspack/generate10g.sh  $awrdir/generate10g.sh
	
}


directory
sarreport
iostatreport
vmreport
mpstatreport
while test 3 -gt 1
do
    sarreport
	iostatreport
	vmreport
	mpstatreport
    sleep 6
done
