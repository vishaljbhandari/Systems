#!/bin/bash
. $WATIR_SERVER_HOME/Scripts/configuration.sh
. $COMMON_MOUNT_POINT/Conf/rangerenv.sh


setScheduler()
{

rm $COMMON_MOUNT_POINT/FMSData/Scheduler/*PID*
> $COMMON_MOUNT_POINT/FMSData/Scheduler/SchedulerFile
crontab -r

}

main()
{
        setScheduler
}

main
