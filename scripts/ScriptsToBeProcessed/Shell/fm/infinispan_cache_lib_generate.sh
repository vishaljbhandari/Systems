cd ../infinispan_distributed_cache_src

> ../src/log/GenerateFmInfinispanCacheLibs.log

currDir=`pwd`
currDir="$currDir/"
destDirRelative=../src/vendor/plugins/fm_infinispan_libs

install_path=$currDir$destDirRelative

./bootstrap && ./configure --enable-64bit --enable-debug --prefix=$install_path >> ../src/log/GenerateFmInfinispanCacheLibs.log 2>&1

status=`echo $?`
if [ $status -eq 0 ]
then
	gmake >> ../src/log/GenerateFmInfinispanCacheLibs.log 2>&1
else
	cd -
	exit 1
fi

status=`echo $?`
if [ $status -eq 0 ]
then
	gmake install >> ../src/log/GenerateFmInfinispanCacheLibs.log 2>&1
else
	cd -
	exit 1
fi

status=`echo $?`
if [ $status -eq 0 ]
then
	echo "Succcesfully created FM Infinispan Library" >> ../src/log/GenerateFmInfinispanCacheLibs.log
	cp ../src/vendor/plugins/fm_infinispan_libs/lib/libfmispndistributedcache.so $RANGERHOME/lib/libfmispndistributedcache_rbin.so
else
	cd -
	exit 1
fi

cd -
