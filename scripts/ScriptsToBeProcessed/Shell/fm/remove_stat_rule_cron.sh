cat $RANGERHOME/RangerData/Scheduler/SchedulerFile | grep -v "StatisticalRuleExecutor" > $RANGERHOME/RangerData/Scheduler/SchedulerFile
crontab -l | grep -v "StatisticalRuleExecutor" > temp
crontab temp
