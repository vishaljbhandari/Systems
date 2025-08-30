#!/bin/bash
#******************************************************************************************************
#* The Script may be used and/or copied with the written permission from
#* Subex Ltd. or in accordance with the terms and conditions stipulated in
#* the agreement/contract (if any) under which the program(s) have been supplied
#*
#* AUTHOR:Saktheesh & Vijayakumar & Mahendra Prasad S
#* Modified: 10-July-2015
#* Usage :./Extract.sh

#* Description:
#*      This script will Take necessary tar balls from Source box to the destination box. 
#******************************************************************************************************

#Configure Extract.conf with development box info before using this script.

logFile=`date|awk '{ print "LOG/" $3 "_" $2 "_" $6 "_NikiraExtract.log" }'`
echo "logFile = $logFile"
REMOVE_PATTERN='*\&%#~!^\%';
ESC_REMOVE_PATTERN='\*\&\%\#\~\!\^\%';
os=`uname`
if [ "$os" == "SunOS" ]
then
	tar_command="gtar"
else
	tar_command="tar"
fi;

#-----------------------------------------------------Functions Start----------------------------------------------------------------------------#

function echo_log()
{
    echo "$@"
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile
}

function log_only()
{
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile
}


NBIT_HOME=`pwd`;
SOURCE_BOX_IP="";
echo_log "Extractor Started .....................................";

function ExportSRCPaths
{
		for i in `cat Extract.conf|grep -v "#"|grep -v "TAR.SOURCE"|grep -v "PATH.SOURCE"`
		do  
		   	export $i
		done;

       	if [ ! -d $SOURCE_NIKIRACLIENT ]
		then
			echo_log "$SOURCE_NIKIRACLIENT doesnt exists...!";
		fi;
		NCdir=`basename $SOURCE_NIKIRACLIENT 2>/dev/null`
		NCPdir=`dirname $SOURCE_NIKIRACLIENT 2>/dev/null`
        
		if [ ! -d $SOURCE_NIKIRAROOT ]
		then
			echo_log "$SOURCE_NIKIRAROOT doesnt exists...!";
		fi;
		NRPdir=`dirname $SOURCE_NIKIRAROOT 2>/dev/null`
		NRdir=`basename $SOURCE_NIKIRAROOT 2>/dev/null`
        
		if [ ! -d $SOURCE_NIKIRATOOLS ]
		then
			echo_log "$SOURCE_NIKIRATOOLS doesnt exists...!";
		fi;
		NTPdir=`dirname $SOURCE_NIKIRATOOLS 2>/dev/null`
		NTdir=`basename $SOURCE_NIKIRATOOLS 2>/dev/null`

		if [ ! -d $SOURCE_RUBYROOT ]
		then
			echo_log "$SOURCE_RUBYROOT doesnt exists...!";
		fi;
		RBPdir=`dirname $SOURCE_RUBYROOT 2>/dev/null`
		RBdir=`basename $SOURCE_RUBYROOT 2>/dev/null`
		
		if [ ! -d $SOURCE_APACHEROOT ]
		then
			echo_log "$SOURCE_APACHEROOT doesnt exists...!";
		fi;
		APPdir=`dirname $SOURCE_APACHEROOT 2>/dev/null`
		APdir=`basename $SOURCE_APACHEROOT 2>/dev/null`
		
        if [ ! -d $SOURCE_BOOST ]
		then                    	
			echo_log "$SOURCE_BOOST doesnt exists...!";
		fi;
		BoostPdir=`dirname $SOURCE_BOOST 2>/dev/null`
		Boostdir=`basename $SOURCE_BOOST 2>/dev/null`
		
		if [ ! -d $SOURCE_DAME ]
		then                                    
            echo_log "$SOURCE_DAME doesnt exists...!";
		fi;
		DamePdir=`dirname $SOURCE_DAME 2>/dev/null`
		Damedir=`basename $SOURCE_DAME 2>/dev/null`
		
		if [ ! -d $SOURCE_DBSETUP ]
		then             
			echo_log "$SOURCE_DBSETUP doesnt exists...!";
		fi;
		DBPdir=`dirname $SOURCE_DBSETUP 2>/dev/null`;
		DBdir=`basename $SOURCE_DBSETUP 2>/dev/null`;

		if [ ! -d $SOURCE_COMMONLIBDIR ]
		then                            
	        echo_log "$SOURCE_COMMONLIBDIR doesnt exists...!";
		fi;
		COMPdir=`dirname $SOURCE_COMMONLIBDIR 2>/dev/null`;
		COMdir=`basename $SOURCE_COMMONLIBDIR 2>/dev/null`;
		
		if [ ! -d $SOURCE_MEMCACHE ]
		then                                        
			echo_log "$SOURCE_MEMCACHE doesnt exists...!";
		fi;
		MemCachePDir=`dirname $SOURCE_MEMCACHE 2>/dev/null`;
		MemCacheDir=`basename $SOURCE_MEMCACHE 2>/dev/null`;

		if [ ! -d $SOURCE_DASHBOARD ]
		then                                            
			echo_log "$SOURCE_DASHBOARD doesnt exists...!";
		fi;
		DashBoardPDir=`dirname $SOURCE_DASHBOARD 2>/dev/null`;
		DashBoardDir=`basename $SOURCE_DASHBOARD 2>/dev/null`;

		if [ ! -d $SOURCE_XML_RPC ]
		then
			echo_log "$SOURCE_XML_RPC doesnt exists...!";
		fi;
		XML_RPCPDir=`dirname $SOURCE_XML_RPC 2>/dev/null`;
		XML_RPCDir=`basename $SOURCE_XML_RPC 2>/dev/null`;

		if [ ! -d $SOURCE_BITMAPHOME ]
		then
        	echo_log "$SOURCE_BITMAPHOME doesnt exists...!";
		fi;
		BITMAPPDir=`dirname $SOURCE_BITMAPHOME 2>/dev/null`;
		BITMAPDir=`basename $SOURCE_BITMAPHOME 2>/dev/null`;

		if [ ! -d $SOURCE_JUDYINSTALLDIR ]
		then
			echo_log "$SOURCE_JUDYINSTALLDIR doesnt exists...!";
		fi;
		JUDYPDir=`dirname $SOURCE_JUDYINSTALLDIR 2>/dev/null`;
		JUDYDir=`basename $SOURCE_JUDYINSTALLDIR 2>/dev/null`;

		if [ ! -d $SOURCE_CURLINSTALLDIR ]
		then
			echo_log "$SOURCE_CURLINSTALLDIR doesnt exists...!";
		fi;
		CURLPDir=`dirname $SOURCE_CURLINSTALLDIR 2>/dev/null`;
		CURLDir=`basename $SOURCE_CURLINSTALLDIR 2>/dev/null`;

		if [ ! -d $SOURCE_YAJLINSTALLDIR ]
		then
			echo_log "$SOURCE_YAJLINSTALLDIR doesnt exists...!";
		fi;
		YAJLPDir=`dirname $SOURCE_YAJLINSTALLDIR 2>/dev/null`;
		YAJLDir=`basename $SOURCE_YAJLINSTALLDIR 2>/dev/null`;

		if [ ! -d $SOURCE_OPENSSLDIR ]
		then
			echo_log "$SOURCE_OPENSSLDIR doesnt exists...!";
		fi;
		SSLPDir=`dirname $SOURCE_OPENSSLDIR 2>/dev/null`;
		SSLDir=`basename $SOURCE_OPENSSLDIR 2>/dev/null`;

		if [ ! -d $SOURCE_PCREDIR ]
		then            
			echo_log "$SOURCE_PCREDIR doesnt exists...!";
		fi;
		PCREPDir=`dirname $SOURCE_PCREDIR 2>/dev/null`;
        PCREDir=`basename $SOURCE_PCREDIR 2>/dev/null`;
		
		if [ ! -d $SOURCE_YAMLCPPDIR ]
		then                
			echo_log "$SOURCE_YAMLCPPDIR doesnt exists...!";
		fi;
		YAMLPDir=`dirname $SOURCE_YAMLCPPDIR 2>/dev/null`;
        YAMLDir=`basename $SOURCE_YAMLCPPDIR 2>/dev/null`;

		if [ ! -d $SOURCE_LIBICONVDIR ]
		then                   
			echo_log "$SOURCE_LIBICONVDIR doesnt exists...!";
		fi;
		LIBICONVPDir=`dirname $SOURCE_LIBICONVDIR 2>/dev/null`;
		LIBICONVDir=`basename $SOURCE_LIBICONVDIR 2>/dev/null`;

		if [ ! -d $SOURCE_AUTOCODEDIR ]
		then                                            
			echo_log "$SOURCE_AUTOCODEDIR doesnt exists...!";
		fi;
		AUTOCODEPDir=`dirname $SOURCE_AUTOCODEDIR 2>/dev/null`;
		AUTOCODEDir=`basename $SOURCE_AUTOCODEDIR 2>/dev/null`;

		if [ ! -d $SOURCE_INFINISPANCPPCLIENTINSTALLDIR ]
		then                                            
			echo_log "$SOURCE_INFINISPANCPPCLIENTINSTALLDIR doesnt exists...!";
		fi;
		INFINISPANPDir=`dirname $SOURCE_INFINISPANCPPCLIENTINSTALLDIR 2>/dev/null`;
		INFINISPANDir=`basename $SOURCE_INFINISPANCPPCLIENTINSTALLDIR 2>/dev/null`;

		if [ ! -d $SOURCE_PROTOBUFINSTALLDIR ]
		then                                            
			echo_log "$SOURCE_PROTOBUFINSTALLDIR doesnt exists...!";
		fi;
		PROTOBUFPDir=`dirname $SOURCE_PROTOBUFINSTALLDIR 2>/dev/null`;
		PROTOBUFDir=`basename $SOURCE_PROTOBUFINSTALLDIR 2>/dev/null`;

}

function GenerateBashrc
{
		echo_log "Generating bashrc.Generated ....";
		>bashrc.Generated
		echo_log "Generating bashrc.Generated ....";
		>bashrc_32.Generated
		cp bashrc.template bashrc.Generated
		cp bashrc_32.template bashrc_32.Generated
		perl -pi -e "s/BOOSTRRR/$Boostdir/g"  bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/DAMERRR/$Damedir/g"  bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/RUBYROOTRRR/$RBdir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/DBSETUPRRR/$DBdir/g"  bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/COMMONLIBDIRRRR/$COMdir/g" bashrc.Generated bashrc_32.Generated 
		perl -pi -e "s/APACHEROOTRRR/$APdir/g" bashrc.Generated bashrc_32.Generated 
		perl -pi -e "s/MEMCACHERRRR/$MemCacheDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/DASHBOARDRRR/$DashBoardDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/XML_RPCRRR/$XML_RPCDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/BITMAPRRR/$BITMAPDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/JUDYRRR/$JUDYDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/CURLRRR/$CURLDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/YAJLRRR/$YAJLDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/OPENSSLRRR/$SSLDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/PCRE_HOMERRR/$PCREDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/YAMLCPPINSTALLDIRRRR/$YAMLDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/LIBICONVRRR/$LIBICONVDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/AUTOCODERRR/$AUTOCODEDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/INFINISPANRRR/$INFINISPANDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/PROTOBUFRRR/$PROTOBUFDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/ANTLRDIRRR/$ANTLRDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/GETTEXTINSTALLDIRRRR/$GETTEXTINSTALLDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/CMAKE_INSTALL_DIRRRR/$CMAKE_INSTALL_Dir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/LIBEVENTINSTALLDIRRRR/$LIBEVENTINSTALLDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/APRINSTALLDIRRRR/$APRINSTALLDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/APRUTILINSTALLDIRRRR/$APRUTILINSTALLDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/APACHELOG4CXXINSTALLDIRRRR/$APACHELOG4CXXINSTALLDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/GMPRRR/$GMPDir/g" bashrc.Generated bashrc_32.Generated
		perl -pi -e "s/MPCRRR/$MPCDir/g" bashrc.Generated bashrc_32.Generated

		if [ "$DATABASE_TYPE" = "db2" ];then
			perl -pi -e "s/\@oracle\@/${REMOVE_PATTERN}/g" bashrc.Generated bashrc_32.Generated
			perl -pi -e "s/\@db2\@//g" bashrc.Generated bashrc_32.Generated
		elif [ "$DATABASE_TYPE" = "oracle" ];then
			perl -pi -e "s/\@oracle\@//g" bashrc.Generated bashrc_32.Generated
			perl -pi -e "s/\@db2\@/${REMOVE_PATTERN}/g" bashrc.Generated bashrc_32.Generated
		else
			echo "DATABASE_TYPE : $DATABASE_TYPE is invalid it can be [oracle|db2]"
		fi

		sed "/$ESC_REMOVE_PATTERN/d" bashrc.Generated > .tmp_bashrc.Generated.$$
		mv .tmp_bashrc.Generated.$$ bashrc.Generated
		sed "/$ESC_REMOVE_PATTERN/d" bashrc_32.Generated > .tmp_bashrc_32.Generated.$$
		mv .tmp_bashrc_32.Generated.$$ bashrc_32.Generated

		os_spec_lib="$OS_SPEC_LIB"
		if [ "$OS_SPEC_LIB" == "" ]
		then
			os_spec_lib="$RUBY_OS_SPEC_LIB"
		fi
		perl -pi -e "s/OS_SPEC_LIB=.*/OS_SPEC_LIB=$os_spec_lib/g" bashrc.Generated bashrc_32.Generated

		rails_gem_version=`rails -v|grep 'Rails'|awk '{ print $2 }'`
		if [ "$rails_gem_version" == "" ]
		then
				rails_gem_version="1.2.3";
		fi;		
		perl -pi -e "s/RAILS_GEM_VERSIONRRR/$rails_gem_version/g" bashrc.Generated bashrc_32.Generated 

		if [ `uname` != "HP-UX" ]
		then
			perl -pi -e "s/[ ]*export[ ]*LD_PRELOAD.*//g" bashrc.Generated bashrc_32.Generated
		fi
		echo_log "Generation of bashrc.Generated is done";
		echo_log "Generation of bashrc_32.Generated is done";
}

function CheckRemoteOrLocal
{
		>SSHToSrc.sh

		if [ "$SOURCE_BOX_IP" != "" ]
		then
				echo_log "Connecting to the Remote box $SOURCE_BOX_IP";
				echo "ssh -l $SOURCE_BOX_USER $SOURCE_BOX_IP <<EOD 2>&1">>SSHToSrc.sh
				echo "bash">>SSHToSrc.sh
		else
				echo_log "Local Box Extraction.....";
				echo "#!/bin/bash">>SSHToSrc.sh
				LOCAL="YES";
		fi		
}

function ModuleValidate
{
		echo_log "Module Validation ....."
		if [ "$ModuleName" != "FMSROOT" -a "$ModuleName" != "FMSTOOLS" -a "$ModuleName" != "FMSCLIENT" -a "$ModuleName" != "ALL" -a "$ModuleName" != "LIBENCODE" ]
		then
				echo_log "Module Name can be [FMSROOT/FMSTOOLS/FMSCLIENT/LIBENCODE]";
				exit 3;
		else if [ "$ModuleName" == "ALL" -a $Exclude -gt 0 ]
		     then
			echo_log "Cannot Exclude ALL modules. Give only one of [FMSROOT/FMSTOOLS/FMSCLIENT]";
			Help
			exit 1
		     fi
		fi
		echo_log "Module Validation Succeeded....."
}

function ExtractNikiraRoot
{
		echo_log "Extracting NikiraRoot..."
		echo "cd $NRPdir">>SSHToSrc.sh
		v=`cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRAROOT"`
		if [ ! -z "$v" -a "$v"!=" " ]; then
			echo "cd">>SSHToSrc.sh
			echo "$tar_command -cvf ~/NikiraRoot.tar sample.txt">>SSHToSrc.sh
			echo "cd $NRPdir">>SSHToSrc.sh
			for i in `cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRAROOT"`
			do
            	path=`echo $i | cut -d '=' -f2`
				echo "$tar_command -rvf ~/NikiraRoot.tar $NRdir/$path -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
			done;
		else
			echo "$tar_command -cvf ~/NikiraRoot.tar $NRdir -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
		fi;

		echo "cd">>SSHToSrc.sh
		echo "$tar_command -rvf NikiraCoreRelease.tar  NikiraRoot.tar">>SSHToSrc.sh 

		echo "rm NikiraRoot.tar">>SSHToSrc.sh

		echo "echo \"TarFile NikiraCoreRelease.tar contains NikiraRoot.tar\" ">>SSHToSrc.sh;
		
}

function ExtractNikiraClient
{
		echo_log "Extracting NikiraClient..."
		echo "cd $NCPdir">>SSHToSrc.sh
		v=`cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRACLIENT"`
		if [ ! -z "$v" -a "$v"!=" " ]; then
			echo "cd">>SSHToSrc.sh
			echo "$tar_command -cvf ~/NikiraClient.tar sample.txt">>SSHToSrc.sh
			echo "cd $NCPdir">>SSHToSrc.sh
			for i in `cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRACLIENT"`
			do
				path=`echo $i | cut -d '=' -f2` 
				echo "$tar_command -rvf ~/NikiraClient.tar $NCdir/$path -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
			done;
		else
            echo "$tar_command -cvf ~/NikiraClient.tar $NCdir -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
		fi;
		
		echo "cd">>SSHToSrc.sh
		echo "$tar_command -rvf NikiraCoreRelease.tar  NikiraClient.tar ">>SSHToSrc.sh 

		echo "rm NikiraClient.tar">>SSHToSrc.sh
		echo "echo \"TarFile NikiraCoreRelease.tar contains NikiraClient.tar\" ">>SSHToSrc.sh;

}

function ExtractNikiraTools
{
	echo_log "Extracting NikiraTools..."
	echo_log "Preparing to Extract Dashboard ($DashBoardDir) from $DashBoardPDir ..."
	echo "echo \"             Extracting  Dashboard\"">>SSHToSrc.sh
	echo_log "cd $DashBoardPDir"
	echo "cd $DashBoardPDir">>SSHToSrc.sh
	echo "$tar_command -cvf ~/Dashboard.tar $DashBoardDir -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
	echo "cd">>SSHToSrc.sh
	echo "$tar_command -rvf NikiraCoreRelease.tar Dashboard.tar">>SSHToSrc.sh
	echo "rm Dashboard.tar">>SSHToSrc.sh
	


	echo_log "Preparing to Extract CommonLib($COMdir) from $COMPdir ..."
	echo "echo \"             Extracting  CommonLib\"">>SSHToSrc.sh
	echo_log "cd $COMPdir">>SSHToSrc.sh
	echo "$tar_command -cvf ~/COMMONLIBDIR.tar $COMdir -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
	echo "cd">>SSHToSrc.sh
	echo "$tar_command -rvf NikiraCoreRelease.tar COMMONLIBDIR.tar">>SSHToSrc.sh
	echo "rm COMMONLIBDIR.tar">>SSHToSrc.sh


	echo_log "Preparing to Extract NikiraTools($NTdir) from $NTPdir..."
	echo "echo \"             Extracting  NikiraTools\"">>SSHToSrc.sh
	echo "cd $NTPdir">>SSHToSrc.sh
	v=`cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRATOOLS"`
   	if [ ! -z "$v" -a "$v"!=" " ]; then
		echo "cd">>SSHToSrc.sh
		echo "$tar_command -cvf ~/NikiraTools.tar sample.txt">>SSHToSrc.sh
		echo "cd $NTPdir">>SSHToSrc.sh
		for i in `cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRATOOLS"`
		do
			path=`echo $i | cut -d '=' -f2`
			echo "$tar_command -rvf ~/NikiraTools.tar $NTdir/$path --exclude='.svn*'">>SSHToSrc.sh
		done;
	else
		echo "$tar_command -cvf ~/NikiraTools.tar $NTdir --exclude='.svn*'">>SSHToSrc.sh
	fi;
	echo "cd">>SSHToSrc.sh
	echo "$tar_command -rvf NikiraCoreRelease.tar NikiraTools.tar">>SSHToSrc.sh
	echo "rm NikiraTools.tar">>SSHToSrc.sh

	echo "echo \"TarFile NikiraCoreRelease.tar contains NikiraTools\" ">>SSHToSrc.sh;

}

function ExtractAll
{
		#Dont remove the following 5 lines,since In solaris SSH will not use first 5 lines.
		echo "###################################################################">>SSHToSrc.sh
		echo "###################################################################">>SSHToSrc.sh
		echo "###################################################################">>SSHToSrc.sh
		echo "###################################################################">>SSHToSrc.sh
		echo "###################################################################">>SSHToSrc.sh
		echo "cd">>SSHToSrc.sh
		echo "echo \"Creating NikiraCoreRelease.tar\"">>SSHToSrc.sh
		echo "echo \"this is sample.txt\">sample.txt">>SSHToSrc.sh
		echo "$tar_command -cvf NikiraCoreRelease.tar sample.txt">>SSHToSrc.sh

		echo_log "Preparing to Extract NikiraRoot($NRdir) from $NRPdir..."
		echo "echo \"             Extracting  NikiraRoot\"">>SSHToSrc.sh
		echo "cd $NRPdir">>SSHToSrc.sh
		v=`cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRAROOT"`
		if [ ! -z "$v" -a "$v"!=" " ]; then
			echo "cd">>SSHToSrc.sh
			echo "$tar_command -cvf ~/NikiraRoot.tar sample.txt">>SSHToSrc.sh
			echo "cd $NRPdir">>SSHToSrc.sh
			for i in `cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRAROOT"`
			do
				path=`echo $i | cut -d '=' -f2`
				echo "$tar_command -rvf ~/NikiraRoot.tar $NRdir/$path -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
			done;
		else
			echo "$tar_command -cvf ~/NikiraRoot.tar $NRdir -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
		fi;
		echo "cd">>SSHToSrc.sh
		echo "$tar_command -rvf NikiraCoreRelease.tar NikiraRoot.tar">>SSHToSrc.sh
		echo "rm NikiraRoot.tar">>SSHToSrc.sh

		echo_log "Preparing to Extract NikiraClient($NCdir) from $NCPdir..."
		echo "echo \"             Extracting  NikiraClient\"">>SSHToSrc.sh
		echo "cd $NCPdir">>SSHToSrc.sh
		v=`cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRACLIENT"`
		if [ ! -z "$v" -a "$v"!=" " ]; then
			echo "cd">>SSHToSrc.sh
			echo "$tar_command -cvf ~/NikiraClient.tar sample.txt">>SSHToSrc.sh
			echo "cd $NCPdir">>SSHToSrc.sh
			for i in `cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRACLIENT"`
			do
				path=`echo $i | cut -d '=' -f2`
				echo "$tar_command -rvf ~/NikiraClient.tar $NCdir/$path -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
			done;
		else
			echo "$tar_command -cvf ~/NikiraClient.tar $NCdir -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
		fi;
		echo "cd">>SSHToSrc.sh
		echo "$tar_command -rvf NikiraCoreRelease.tar NikiraClient.tar">>SSHToSrc.sh
		echo "rm NikiraClient.tar">>SSHToSrc.sh

		echo_log "Preparing to Extract DBSetup($DBdir) from $DBPdir..."
		echo "echo \"             Extracting  DBSetup\"">>SSHToSrc.sh
		echo "cd $DBPdir">>SSHToSrc.sh
		echo "$tar_command -cvf ~/DBSetup.tar $DBdir -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
		echo "cd">>SSHToSrc.sh
		echo "$tar_command -rvf NikiraCoreRelease.tar DBSetup.tar">>SSHToSrc.sh
		echo "rm DBSetup.tar">>SSHToSrc.sh

		echo_log "Preparing to Extract CommonLib($COMdir) from $COMPdir ..."
		echo "echo \"             Extracting  CommonLib\"">>SSHToSrc.sh
		echo_log "cd $COMPdir">>SSHToSrc.sh
		echo "$tar_command -cvf ~/COMMONLIBDIR.tar $COMdir -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
		echo "cd">>SSHToSrc.sh
		echo "$tar_command -rvf NikiraCoreRelease.tar COMMONLIBDIR.tar">>SSHToSrc.sh
		echo "rm COMMONLIBDIR.tar">>SSHToSrc.sh

		echo_log "Preparing to Extract Dashboard ($DashBoardDir) from $DashBoardPDir ..."
		echo "echo \"             Extracting  Dashboard\"">>SSHToSrc.sh
		echo_log "cd $DashBoardPDir"
		echo "cd $DashBoardPDir">>SSHToSrc.sh
		echo "$tar_command -cvf ~/Dashboard.tar $DashBoardDir -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
		echo "cd">>SSHToSrc.sh
		echo "$tar_command -rvf NikiraCoreRelease.tar Dashboard.tar">>SSHToSrc.sh
		echo "rm Dashboard.tar">>SSHToSrc.sh
		
		echo_log "Preparing to Extract NikiraTools($NTdir) from $NTPdir..."
		echo "echo \"             Extracting  NikiraTools\"">>SSHToSrc.sh
		echo "cd $NTPdir">>SSHToSrc.sh
		v=`cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRATOOLS"`
		if [ ! -z "$v" -a "$v"!=" " ]; then
			echo "cd">>SSHToSrc.sh
			echo "$tar_command -cvf ~/NikiraTools.tar sample.txt">>SSHToSrc.sh
			echo "cd $NTPdir">>SSHToSrc.sh
			for i in `cat Extract.conf|grep -v "#"|grep "TAR.SOURCE_NIKIRATOOLS"`
			do
				path=`echo $i | cut -d '=' -f2`
				echo "$tar_command -rvf ~/NikiraTools.tar $NTdir/$path --exclude='.svn*'">>SSHToSrc.sh
			done;
		else
			echo "$tar_command -cvf ~/NikiraTools.tar $NTdir --exclude='.svn*'">>SSHToSrc.sh
		fi;
		echo "cd">>SSHToSrc.sh
		echo "$tar_command -rvf NikiraCoreRelease.tar NikiraTools.tar">>SSHToSrc.sh
		echo "rm NikiraTools.tar">>SSHToSrc.sh


		echo "cd">>SSHToSrc.sh;
		echo "cksum NikiraCoreRelease.tar >CheckSumNikira.txt">>SSHToSrc.sh

		if [ "$SOURCE_BOX_IP" == "" ]
		then
			echo_log "Taring $NBIT_HOME ........"
			echo "cd $NBIT_HOME">>SSHToSrc.sh
			echo "echo \"Preparing to              Extracting   NBIT\"">>SSHToSrc.sh
			echo "cd ..">>SSHToSrc.sh
			echo "$tar_command -cvf ~/NBIT.tar NBIT -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'">>SSHToSrc.sh
			echo "cksum ~/NBIT.tar>CheckSumNBIT.txt">>SSHToSrc.sh
			echo "exit">>SSHToSrc.sh
			echo "EOD">> SSHToSrc.sh 
		fi;		

		echo_log "TarFile NikiraCoreRelease.tar contains NikiraRoot.tar NikiraClient.tar DBSetup.tar COMMONLIBDIR.tar NikiraTools";

}

function ExecuteExtractScript
{
		chmod 777 SSHToSrc.sh;

		./SSHToSrc.sh>tlog 2>&1

		cat tlog|egrep "Error |errors " >/dev/null 2>&1

		if [ "$?" == "0" ]
		then
				cat tlog
				echo_log ""
				echo_log "Warning:Extraction is Not succeeded for the Above Errors(if errors are not severe then it is successfull......";
				echo_log "logfile:$NBIT_HOME/tlog";
				Err=1;		
		#		exit 1;
		fi;
}



function DoFtp
{
		echo_log  "	Do FTP connection to Source Box and Get the tar file If it is not Local machine.."
		if [ "$Err" == "1" ]
		then
			echo_log "Do You want to proceed this transfer <default N> [Y/N]";
			read $q;
			log_only "$q";
			if [ "$q" == "N" -o "$q" == "n" ]
			then
				echo_log "transfer skipped... Extraction Over in local";
				exit 5;
			fi;	
		fi;	
		rm NikiraCoreRelease.tar  >/dev/null 2>&1;
		rm NBIT.tar  >/dev/null 2>&1;
		
		ftp  -n -i <<EOF
		passive
		open $SOURCE_BOX_IP 
		user $SOURCE_BOX_USER $SOURCE_BOX_PASSWORD
		bi
		lcd
		get NikiraCoreRelease.tar 
		get CheckSumNikira.txt
		get NBIT.tar
		bye
EOF
		echo_log "Extraction of the nikira from source box to Destination box is done"
		echo_log "NikiraCoreRelease.tar is available in $HOME dir";
}

function Help
{
		echo_log "Usage: $PROG_NAME {[-m ModuleName] [-e ExcludeModuleName]}";
		echo_log "or    $PROG_NAME -h ";
		echo_log "or    $PROG_NAME -b ";
		echo_log "-m/-e Module Name [FMSROOT/FMSTOOLS/FMSCLIENT] Default ALL";
		echo_log "-b to Generate bashrc.Generated and bashrc_32.Generated";
		echo_log "-h Help.";
}

function Extract
{
    if [ "$ModuleName" == "LIBENCODE" ]
	then
		echo "Your shiping only libencode to DB Server... ";
		echo_log "Extracting libencode to LIBENCODE folder..."
		echo "cd $SOURCE_NIKIRAROOT">>SSHToSrc.sh
		echo "cd lib">>SSHToSrc.sh
		echo "mkdir -p $NBIT_HOME/LIBENCODE">>SSHToSrc.sh
		echo "cp libdbencode.so $NBIT_HOME/LIBENCODE/libencode.so">>SSHToSrc.sh
		echo_log "Extracting libencode to LIBENCODE folder is done!!..."
		echo "cd $NBIT_HOME">>SSHToSrc.sh
		echo "echo \"this is sample.txt\">sample.txt">>SSHToSrc.sh
		echo "$tar_command -cvf ~/NikiraCoreRelease.tar sample.txt">>SSHToSrc.sh
		echo "rm sample.txt">>SSHToSrc.sh
		echo "$tar_command -cvf ~/LIBENCODE.tar LIBENCODE ">>SSHToSrc.sh
		echo "cd">>SSHToSrc.sh
		echo "$tar_command -rvf NikiraCoreRelease.tar LIBENCODE.tar">>SSHToSrc.sh
		echo "rm LIBENCODE.tar">>SSHToSrc.sh
		echo "cksum NikiraCoreRelease.tar >CheckSumNikira.txt">>SSHToSrc.sh
        return
	fi;


	echo "echo \"this is sample.txt\">sample.txt">>SSHToSrc.sh
	echo "$tar_command -cvf ~/NikiraCoreRelease.tar sample.txt">>SSHToSrc.sh
	if [ $Exclude -gt 0 ]
	then
		if [ "$ModuleName" == "FMSROOT" ]
		then
			ExtractNikiraTools;
			ExtractNikiraClient;				
		else
			if [ "$ModuleName" == "FMSCLIENT" ]
			then
				ExtractNikiraTools;
				ExtractNikiraRoot;
			else
				if [ "$ModuleName" == "FMSTOOLS" ]
				then
					ExtractNikiraClient;				
					ExtractNikiraRoot;
				fi
			fi
		fi
	else 
		if [ "$ModuleName" == "ALL" ]
		then
			
			ExtractAll;
		else
			if [ "$ModuleName" == "FMSROOT" ]
			then
				ExtractNikiraRoot;
			else
				if [ "$ModuleName" == "FMSCLIENT" ]
				then
					ExtractNikiraClient;				
				else
					if [ "$ModuleName" == "FMSTOOLS" ]
					then
						ExtractNikiraTools;
					fi
				fi
			fi
		fi
	fi
	echo "cd">>SSHToSrc.sh;
	echo "cksum NikiraCoreRelease.tar >CheckSumNikira.txt">>SSHToSrc.sh
	echo "rm sample.txt">>SSHToSrc.sh

	if [ "$SOURCE_BOX_IP" == "" ]
	then
			echo "exit">>SSHToSrc.sh
			echo "EOD">> SSHToSrc.sh 
	fi;		
}


function Transfer
{
		if [ "$LOCAL" != "YES" ]
		then
			DoFtp;	
		else
			echo_log "Extraction of the nikira from the local box is done";
			echo_log "NikiraCoreRelease.tar is available in $HOME dir";
		fi;

		exit;
}

#-----------------------------------------------------Functions End----------------------------------------------------------------------------#


#Main Starts Here ...........................
PROG_NAME=$0;
ModuleName="ALL";
Exclude=0;
while getopts m:e:bh OPTION
do
        case ${OPTION} in
		b)ExportSRCPaths;GenerateBashrc;exit 0;;
                m) ModuleName="${OPTARG}" ;;
                e) ModuleName="${OPTARG}"
		   Exclude=1 ;;
                h)Help;exit 0;;
                \?)Help;exit 2;;
        esac
done
echo "Modules=$ModuleName"

ModuleValidate;
ExportSRCPaths;
GenerateBashrc;
CheckRemoteOrLocal;
Extract;
ExecuteExtractScript;
Transfer;

#Main Ends Here ...........................
