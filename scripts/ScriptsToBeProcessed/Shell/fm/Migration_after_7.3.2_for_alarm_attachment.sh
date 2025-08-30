#!/usr/bin/env bash

GetInput()
{
    display=$1
    Input=""
    while [ "$Input" != "Y" ] && [ "$Input" != "N" ] &&  [ "$Input" != "y" ] && [ "$Input" != "n" ];
    do
        read -e -d';' -p"$display [Y/N] ?" -n 1 Input;
    done
    return $([ "$Input" == "Y" ] || [ "$Input" == "y" ]);
}

echo ""
echo "NB : Need to run 'generate_files.sh' script in Client/src before running this script."
echo "     Please make sure that backup of previous Nikira Alarm attachments folder has been taken."
echo ""

GetInput "Do you want to continue"
Continue=$?

if [ $Continue -ne 0 ]
then
	exit 0
fi

echo ""
echo "Please Enter Previous Nikira TAG CheckOut Path"
read PrevTag

if [ ! -d $PrevTag ]
then
echo ""
	echo "Directory path: '$PrevTag'  does not exists!"
	exit 1 ;
fi

if [ ! -d $PrevTag/Client/src/public/Attachments ]
then
	echo ""
	echo "No any Attachments (folder) available this tag"
	exit 1 ;
fi

echo ""
echo "Please Enter Current Nikira TAG CheckOut Path"
read CurrentTag

if [ ! -d $CurrentTag ]
then
	echo ""
	echo "Directory path: '$CurrentTag'  does not exists! "
	exit 1 ;
fi

if [ ! -d $CurrentTag/Client/src/private/Attachments ]
then
	echo ""
	echo "Please run 'generate_files.sh' to continue. "
	exit 1 ;
fi

mv $CurrentTag/Client/src/private/Attachments $CurrentTag/Client/src/private/Back.Attachments
cp -R $PrevTag/Client/src/public/Attachments $CurrentTag/Client/src/private/Attachments 

echo ""
echo ""
ruby -e "puts \"\x1b[38;32mMigrated Attachments Successfully ...!\x1b[38;0m\""
echo ""

