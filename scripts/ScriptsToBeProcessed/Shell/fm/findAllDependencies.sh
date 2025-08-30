#!/bin/bash
#This script identifies the dependent libraries used by Nikira & its tools.
#As of now, we are considering the following libraries as dependents. i
#If required/identified any new dependency, one can add to the following list.
# 	libz.so
#	libiconv.so
#	libdb.so
#	libgdbm.so
#	libpthread.so
#	libcrypt_i.so
#	libgcc_s.so
#	libstdc++.so
#	libcurses.so
#	libssl.so
#	libcrypto.so
#	readline.so

if [ "$RUBYROOT" == "" -o "$RUBY_OS_SPEC_LIB" == "" -o "$COMMONLIBDIR" == "" -o "$APACHEROOT" == "" -o "$FMSROOT" == "" ]; then
	echo "\$RUBYROOT or \$RUBY_OS_SPEC_LIB or \$COMMONLIBDIR or \$APACHEROOT or \$FMSROOT is not!!!!"
	echo "Please set the above Environment variables and then try again....."
	exit 1
fi
if [ ! -d $COMMONLIBDIR/lib32 ]
then
	mkdir -p $COMMONLIBDIR/lib32
fi
file_types="executable"
bit64Name="ELF-64"
if [ `uname` = 'HP-UX' ]
 then
	file_types="$file_types|shared object file"
else if [ `uname` = 'SunOS' ] 
	then 
		file_types="$file_types|dynamic lib"
		bit64Name="64-bit"
	else if [ `uname` == 'AIX' ]
             then
                bit64Name="64-bit"
                commonFiles="$commonFiles"
             fi
	fi

fi
dirs="$RUBYROOT/lib/ruby/1.8/$RUBY_OS_SPEC_LIB $APACHEROOT/lib $APACHEROOT/bin $FMSROOT/sbin $FMSROOT/lib $COMMONLIBDIR"
for dir_name in $dirs
do
	echo "Finding dependencies in $dir_name ..............."
	for binaryfile in `find $dir_name |xargs file|egrep "dynamic lib|shared object|executable"|egrep -v "script|archive"|awk '{print $1}'|cut -d":" -f1`
	do
		for library in `ldd $binaryfile 2>/dev/null|egrep "=>"|grep -v "(file not found)"|egrep "libz|libiconv|libdb|libgdb|libcrypt_i|libgcc_s|libcurses|libssl|libcrypto|readline"|awk '{print $3}'|sort -u` 
		do
			bit_size=`file $library|egrep $bit64Name|wc -l`
			subdir='lib32'
			if [ $bit_size -ne 0 ]
			then
				subdir=''
			fi
			libname=`basename $library`
			if [ ! -f "$COMMONLIBDIR/$subdir/$libname" ] 
			then
				echo "cp $library $COMMONLIBDIR/$subdir/$libname"
				cp $library $COMMONLIBDIR/$subdir/$libname
			fi
		done
	done
done
