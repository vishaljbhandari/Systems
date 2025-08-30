#! /bin/bash


. $WATIR_SERVER_HOME/Scripts/configuration.sh

noOfIterations=60
sleepTimePerIteration=".5"

loadFixtures()
{
i=1
noOfFixtures=${#fixtureFileName[*]}

while [ $i -le $noOfFixtures ]
do
	if [ -f $WATIR_SERVER_HOME/Scripts/Fixture/FixtureFiles/${fixtureFileName[$i]} ] # if fixture file exists
	then
		echo "Loading fixture ${fixtureFileName[$i]} at Level $i ..."
		
		if [ $DB_TYPE == "1" ]
		then
		sqlplus -s $DB_SETUP_USER/$DB_SETUP_PASSWORD @$WATIR_SERVER_HOME/Scripts/Fixture/FixtureFiles/${fixtureFileName[$i]} >/dev/null
		else
		clpplus -nw -s $DB2_SERVER_USER/$DB2_SERVER_PASSWORD@$DB2_SERVER_HOST:$DB2_INSTANCE_PORT/$DB2_DATABASE_NAME @$WATIR_SERVER_HOME/Scripts/Fixture/FixtureFiles/${fixtureFileName[$i]} > /dev/null
		fi
		if [ $i -eq $noOfFixtures ] # if it is last fixture then run programmanager
		then	

			while [ $noOfIterations -gt 0 ]
			do
			        if [[ -e $WATIR_SERVER_HOME/Scripts/Fixture/tmp/flagToRunProgrammanager && -e $WATIR_SERVER_HOME/Scripts/Fixture/tmp/sequenceFlag.lst ]] ; then
							bash $WATIR_SERVER_HOME/Scripts/RunProgramManager.sh
			                exit 0
			        else
               				 sleep $sleepTimePerIteration
			        fi

		        noOfIterations=`expr $noOfIterations - 1`
			done

		fi
	else
		echo "Fixture file does not exist for ${fixtureFileName[$i]}"
		exit 2
	fi

	i=`expr $i + 1`
done

return 0
}

main()
{
i=1

while [ $# -gt 0 ]
do
    fixtureFileName[$i]="$1.sql"
    shift
    i=`expr $i + 1`
done

while [ $noOfIterations -gt 0 ]
do
    if [ ! -f $WATIR_SERVER_HOME/Scripts/Fixture/tmp/flagToRunProgrammanager ]
    then
        sleep $sleepTimePerIteration
    else
        loadFixtures
        exit 0
    fi

    noOfIterations=`expr $noOfIterations - 1`
done

echo "Unable to load fixture as Signal file missing !!!!!!"
return 0
}


main $*

