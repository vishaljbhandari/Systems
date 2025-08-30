
#! /bin/bash

os=`uname`
if [ "$os" == "SunOS" ]
then
    patch_command="gpatch"
    tar_command="gtar"
else
    patch_command="patch"
    tar_command="tar"
fi

GetInput()
{
	display=$1
		Input=""
		while [ "$Input" != "Y" ] && [ "$Input" != "N" ] &&  [ "$Input" != "y" ] && [ "$Input" != "n" ];
	do
		read -e -d';' -p"$display [Y/N] ?" -n 1 Input;
	done

	if [ "$Input" == "Y" -o "$Input" == "y" ]
	then
		return 1
	else
		return 0
	fi

}

currentdir=`pwd`
echo "Please Enter Nikira CheckOut Path"
read CHECKOUTPATH

if [ -d $CHECKOUTPATH ]
then
    cd $CHECKOUTPATH
	echo `pwd`

	mv Mobile Mobile.bkp.SP3
	mv ROC_FM_Upgrade ROC_FM_Upgrade.bkp.SP3
	mv DBInstaller DBInstaller.bkp.SP3
	
    $tar_command -zxvf ServicePacks/ServicePack-4/newFiles.tar.gz
    $tar_command -zxvf ServicePacks/ServicePack-4/Mobile.tar.gz
    $tar_command -zxvf ServicePacks/ServicePack-4/ROC_FM_Upgrade.tar.gz
    $tar_command -zxvf ServicePacks/ServicePack-4/AppNotes.tar.gz
    $tar_command -zxvf ServicePacks/ServicePack-4/UsageDocs.tar.gz
    $tar_command -zxvf ServicePacks/ServicePack-4/DBInstaller.tar.gz


	$patch_command -p0 -i ServicePacks/ServicePack-4/client_01.patch
	$patch_command -p0 -i ServicePacks/ServicePack-4/server_01.patch
	$patch_command -p0 -i ServicePacks/ServicePack-4/rocfm_spark_01.patch
	$patch_command -p0 -i ServicePacks/ServicePack-4/dsm_seed_schema_01.patch
	cp ServicePacks/ServicePack-4/nikira-1.0.0.jar ROC_FM_Spark/server-modular/fms_libraries
	
	$patch_command -p0 -i ServicePacks/ServicePack-4/client_02.patch
	
	cp ServicePacks/ServicePack-4/en_help_files.tar.gz Client/src/resources/en_help_files.tar.gz
	cp ServicePacks/ServicePack-4/ROCFraudManagementUserManual.pdf Client/src/public/help_files/PDF/ROCFraudManagementUserManual.pdf

	$patch_command -p0 -i ServicePacks/ServicePack-4/client_03.patch
	cp ServicePacks/ServicePack-4/Visualisation_usage_doc.docx UsageDocs/
	
	$patch_command -p0 -i ServicePacks/ServicePack-4/client_04.patch
else
    echo "Please Enter Valid Path"
fi
