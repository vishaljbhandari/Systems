#! /bin/bash
patchDir=`pwd`

os=`uname`

if [ "$os" == "SunOS" ]
 then
    patch_command="gpatch"
 else
    patch_command="patch"
fi

echo "Mesg"
read TARGETPATH

if [ -d  $TARGETPATH ]
then
	if [ -d  $TARGETPATH/Server ]
	then

    	cp -r sourcefile destinationfile
    	$patch_command -p0 -i Patches/patchfile

	
	else
		echo "Server Directory Is Missing"
		echo "Please Enter Valid Path"
	fi	
	cd $patchDir
else
    cd $patchDir
    echo "Please Enter Valid Path"
fi

