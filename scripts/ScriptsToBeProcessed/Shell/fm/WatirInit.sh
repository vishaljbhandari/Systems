#!/bin/bash
#set -x
. $WATIR_HOME/Scripts/Server/configuration.sh

a=`pwd`
ssh -l $USER $HOST rm -rf $WATIR_SERVER_HOME
ssh -l $USER $HOST mkdir -p $WATIR_SERVER_HOME/Scripts
ssh -l $USER $HOST mkdir -p $WATIR_SERVER_HOME/BringNikiraSetup

# echo $WATIR_SERVER_HOME_HOME=$WATIR_HOME
# echo USER=$USER
# echo HOST=$HOST

cd $WATIR_HOME/Scripts/Server
for i in *
do
	scp $i $USER@$HOST:$WATIR_SERVER_HOME/Scripts/$i
	ssh -l $USER $HOST chmod 775 $WATIR_SERVER_HOME/Scripts/$i
done
sleep 2

UNAME=`ssh -l $USER $HOST uname`

for i in *
do
	case $UNAME in
		SunOS ) ssh -l $USER $HOST dos2unix -437 $WATIR_SERVER_HOME/Scripts/$i $WATIR_SERVER_HOME/Scripts/$i ;;
		Linux ) ssh -l $USER $HOST dos2unix $WATIR_SERVER_HOME/Scripts/$i ;;
		HP-UX ) ssh -l $USER $HOST dos2ux $WATIR_SERVER_HOME/Scripts/$i $WATIR_SERVER_HOME/Scripts/$i ;;
		AIX   ) ssh -l $USER $HOST perl -pi -e 's/\r\n/\n/g' $WATIR_SERVER_HOME/Scripts/*.* ;;
    esac
	echo "dos2unix: converting file ${i} to UNIX format..."
done

cd $WATIR_HOME/CIB
for i in *
do
	scp $i $USER@$HOST:$WATIR_SERVER_HOME/BringNikiraSetup/$i
	ssh -l $USER $HOST chmod 775 $WATIR_SERVER_HOME/BringNikiraSetup/$i
done
sleep 2


for i in *
do
	case $UNAME in
        SunOS ) ssh -l $USER $HOST dos2unix -437 $WATIR_SERVER_HOME/BringNikiraSetup/$i $WATIR_SERVER_HOME/BringNikiraSetup/$i ;;
		Linux ) ssh -l $USER $HOST dos2unix $WATIR_SERVER_HOME/BringNikiraSetup/$i ;;
		HP-UX ) ssh -l $USER $HOST dos2ux $WATIR_SERVER_HOME/BringNikiraSetup/$i $WATIR_SERVER_HOME/BringNikiraSetup/$i ;;
		AIX   ) ssh -l $USER $HOST perl -pi -e 's/\r\n/\n/g' $WATIR_SERVER_HOME/BringNikiraSetup/*.* ;;
	esac
	echo "dos2unix: converting file ${i} to UNIX format..."
done

if [ $UNAME == "SunOS" ]
then
	nawk=`ssh -l $USER $HOST which nawk`
	ssh -l $USER $HOST ln -s $nawk awk
fi
	
ssh -l $USER $HOST chmod +x $WATIR_SERVER_HOME/Scripts/*.sh
ssh -l $USER $HOST chmod +x $WATIR_SERVER_HOME/BringNikiraSetup/*.*
cd $a
