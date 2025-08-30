cp $HOME/RangerDatasource/Customization/DU-Dubai/TestData/tibcodata $RANGERHOME/RangerData/TIBCO/
touch   $RANGER5.4HOME/RangerData/TIBCO/success/tibcodata

. $HOME/RangerDatasource/Customization/DU-Dubai/Scripts/tibcoparser.sh& > /dev/null
sleep 20
kill -9 $!

sdiff  $RANGER5.4HOME/RangerData/DataSourceSubscriberData/tibcodata $HOME/RangerDatasource/Customization/DU-Dubai/TestData/tibcoresult > /dev/null

if [ $? -eq 0 ]
then
	echo "Auto Testing succesful"
else
	echo "Auto Testing failed"
fi
  









