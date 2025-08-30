#! /bin/bash


OldSparkVersion=$1
NewSparkVersion=$2

MergeScript()
{

	if [ "$OldSparkVersion" != "" ] && [ "$NewSparkVersion" != "" ]
		then
			ModifyUnixScriptFiles
			ModifyWindowsBatFiles
			UpdateBuildXMl
			UpdateCodeGenerator
			CopyConfigFiles
			UpdateIncludeConf
	else
		echo "Invalid Arguments: Usage --> ./MergeScript [Old Spark Version] [NewSparkVersion] "
	fi

}

ModifyUnixScriptFiles()
{	
# copy all sh files
cd scripts/unix
for f in $(find . -name '*.sh')  ; do cp ../../vendor/spark/bin/$f $f;  done

echo "$(cat  PreSilentInstallationTasks.sh) \$3 \$4">PreSilentInstallationTasks.sh

echo "$(cat  tc.sh)  -HIBERNATEPROPERTIES_DC='../config/DistributedCacheManagerResources/hibernate_dc.cfg.xml'">tc.sh

sed -i 's#../config:#../lib/patches-1.0.0.jar:../lib/nikira-1.0.0.jar:../config:#g'  `find . -name '*.sh'`
sed -i 's#../lib/jboss-logging-3.1.3.GA.jar:##g'  `find . -name '*.sh'`
sed -i 's#../lib/jgroups-all-2.2.8.jar:##g'  `find . -name '*.sh'`
sed -i 's#../config:#../config:../lib/jboss-logging-3.1.3.GA.jar:#g'  `find . -name '*.sh'`
}


ModifyWindowsBatFiles()
{
cd -
cd scripts/windows
for f in $(find . -name '*.bat')  ; do cp ../../vendor/spark/bin/$f $f;  done

sed -i 's#../config;#../lib/patches-1.0.0.jar;../lib/nikira-1.0.0.jar;../config;#g'  `find . -name '*.bat'`
sed -i 's#../lib/jgroups-all-2.2.8.jar;##g'  `find . -name '*.bat'`
sed -i 's#../lib/jboss-logging-3.1.3.GA.jar;##g'  `find . -name '*.bat'`
sed -i 's#../config;#../config;../lib/jboss-logging-3.1.3.GA.jar;#g'  `find . -name '*.bat'`
}

UpdateBuildXMl()
{
cd - 
sed -i "s#$OldSparkVersion#$NewSparkVersion#g"  `find . -name 'build.xml'`
}

UpdateCodeGenerator()
{
cd scripts/unix

sed -i "s#$OldSparkVersion#$NewSparkVersion#g"  `find . -name 'codegenerator.sh'`
sed -i "s#$OldSparkVersion#$NewSparkVersion#g"  `find . -name 'multicachequeryexecutor.sh'`
}

CopyConfigFiles()
{

cd -
cd scripts/config
for f in $(find . -name '*.conf' -o -name '*.conf.default')  ; do cp ../../vendor/spark/config/$f $f;  done

}
UpdateIncludeConf()
{
cd -
cd scripts/config	
x=1
wlist=`awk --field-separator '='  '{print $2}' include.conf`

wlist="./lib/patches-1.0.0.jar ./lib/nikira-1.0.0.jar $wlist"

echo "#Add the classpath relative to deploy folder">include.conf

for w in $wlist
do 
    echo "wrapper.java.classpath.$x=$w" >>include.conf
    x=`expr $x + 1`
done

}


MergeScript

