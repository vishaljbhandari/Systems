#! /bin/bash
echo "Please Enter Nikira CheckOut Path"
read CHECKOUTPATH
if [ -d  $CHECKOUTPATH ]
then
    cd $CHECKOUTPATH
    echo `pwd`
	 patch -p0 -i Patches/Patch_7/HotFix_1/hotfix_1.patch
	 patch -p0 -i Patches/Patch_7/HotFix_1/security-fixes_hotfix.patch
	 patch -p0 -i Patches/Patch_7/HotFix_1/hotfix_2.patch
	 patch -p0 -i Patches/Patch_7/HotFix_1/named_filter.patch

	 cp Patches/Patch_7/HotFix_1/extended_log_config.rb Client/notificationmanager/
	
else
    echo "Please Enter Valid Path"
fi
