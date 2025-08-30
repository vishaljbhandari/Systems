PROG_NAME=$0;
NBIT_HOME=`pwd`;

if [ -d $NBIT_HOME/PATCHES/NIKIRACLIENT ]
then
	rm -rf $NBIT_HOME/PATCHES/NIKIRACLIENT
fi;

if [ -d $NBIT_HOME/PATCHES/NIKIRATOOLS ]
then
	rm -rf $NBIT_HOME/PATCHES/NIKIRATOOLS
fi;

if [ -d $NBIT_HOME/PATCHES/NIKIRAROOT ]
then
	rm -rf $NBIT_HOME/PATCHES/NIKIRAROOT
fi;

os=`uname`
if [ "$os" == "SunOS" ]
then
	tar_command="gtar"
else
	tar_command="tar"
fi;

logFile=`date|awk '{ print "LOG/" $3 "_" $2 "_" $6 "_NBIT_PatchExtractor.log" }'`
echo "logFile = $logFile"

#-----------------------------------------------------Functions Start----------------------------------------------------------------------------#

function echo_log
{
    echo "$@"
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile
}

function log_only
{
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile
}

function Validations
{
		echo_log "Validating the inputs ......";

		if [ "$ModuleName" == "" -a "$VersionName" == "" ]
		then
			echo_log "Mention the module name, Module Name can be [FMSROOT/FMSTOOLS/FMSCLIENT] and Release Name is Not Mentioned";
		exit 5;
		fi;

		if [ "$ModuleName" != "FMSROOT" -a "$ModuleName" != "FMSTOOLS" -a "$ModuleName" != "FMSCLIENT" ]
		then
				echo_log "Module Name can be [FMSROOT/FMSTOOLS/FMSCLIENT]";
				exit 4;
		fi

		if [ "$VersionName" == "" ]
		then
			echo_log    "Release Name is Not Mentioned"
			exit 3;
	    fi;
		echo_log "Validation Done ......";
}

function ExportSRCPaths
{
		echo_log "Exporting Source Machine Paths...."
		for i in `cat Extract.conf|grep -v "#"|grep -v "TAR.SOURCE"|grep -v "PATH.SOURCE"`
		do
			export $i
		done;
		echo_log "Exporting Source Machine Paths Done ...."
}

function ExtractNikiraRootPatch
{
		echo_log "Extracting the NikiraRoot Patch.....";
		if [ ! -d $SOURCE_NIKIRAROOT  ]
		then
				echo_log "Directory $SOURCE_NIKIRAROOT is not exist. Edit NBIT/Extract.conf for correct path";
				exit 5;
		fi;
		
		echo_log "cd $SOURCE_NIKIRAROOT"; 
		cp $NBIT_HOME/PatchFiles.conf $SOURCE_NIKIRAROOT/
		cd $SOURCE_NIKIRAROOT		
		cat PatchFiles.conf|grep -v "^#"|xargs $tar_command -cvf $VersionName.tar -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'>$NBIT_HOME/tlog 2>&1
		cat $NBIT_HOME/tlog|grep "^Error"

		if [ "$?" == "0" ]
		then
				echo_log "Extraction Failed For the Above Errors.. Exiting....";
				exit 1;
		fi;

		echo_log "PATCH is available in $SOURCE_NIKIRAROOT filename:$VersionName.tar"

}

function ExtractNikiraClientPatch
{
		echo_log "Extracting the NikiraClient Patch.....";
		if [ ! -d $SOURCE_NIKIRACLIENT  ]
		then
				echo_log "Directory $SOURCE_NIKIRACLIENT is not exist. Edit NBIT/Extract.conf for correct path";
				exit 5;
		fi;
		
		echo_log "cd $SOURCE_NIKIRACLIENT"; 
		cp $NBIT_HOME/PatchFiles.conf $SOURCE_NIKIRACLIENT/
		cd $SOURCE_NIKIRACLIENT		
		cat PatchFiles.conf|grep -v "^#"|xargs $tar_command -cvf $VersionName.tar -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'>$NBIT_HOME/tlog
		cat $NBIT_HOME/tlog|grep "^Error">/dev/null 2>&1

		if [ "$?" == "0" ]
		then
				echo_log "Extraction Failed For the Above Errors.. Exiting....";
				exit 1;
		fi;

		echo_log "PATCH is available in $SOURCE_NIKIRACLIENT filename:$VersionName.tar"

}

function ExtractNikiraToolsPatch
{
		echo_log "Extracting the NikiraTools Patch.....";
		echo_log "Enter the parent directory of this Tool; eg: if $HOME/RUBYROOT is the tool then give $HOME as input"
		read SOURCE_NIKIRATOOLS;
		log_only "$SOURCE_NIKIRATOOLS" ;	
		if [ ! -d $SOURCE_NIKIRATOOLS ]
		then
		echo_log "Parent Directory Mentioned is wrong"
		exit 7;
		fi;	
		cp PatchFiles.conf $SOURCE_NIKIRATOOLS;

		echo_log "cd $SOURCE_NIKIRATOOLS"; 
		cp $NBIT_HOME/PatchFiles.conf $SOURCE_NIKIRATOOLS/
		cd $SOURCE_NIKIRATOOLS		
		cat PatchFiles.conf|grep -v "^#"|xargs $tar_command -cvf $VersionName.tar -X $NBIT_HOME/ExcludeFiles.conf --exclude='.svn*'>$NBIT_HOME/tlog
		cat $NBIT_HOME/tlog|grep "^Error">/dev/null 2>&1

		if [ "$?" == "0" ]
		then
				echo_log "Extraction Failed For the Above Errors.. Exiting....";
				exit 1;
		fi;

		echo_log "PATCH is available in $SOURCE_NIKIRATOOLS filename:$VersionName.tar"
		exit 0;
		cd $n;
		cat PatchFiles.conf|$tar_command -cvf $VersionName.tar
		echo_log "PATCH is available in $n filename:$VersionName.tar";

}

function CheckAndExtract
{
		if [ "$ModuleName" == "FMSROOT" ]
		then
				ExtractNikiraRootPatch;
				exit 0;
		fi;		

		if [ "$ModuleName" == "FMSCLIENT" ]
		then
				ExtractNikiraClientPatch;
				exit 0;
		fi;		

		if [ "$ModuleName" == "FMSTOOLS" ]
		then
				ExtractNikiraToolsPatch;
				exit 0;
		fi;		

}

function Help
{
		echo_log "Usage: $PROG_NAME -m ModuleName -v ReleaseName ";
		echo_log "or    $PROG_NAME -h ";
		echo_log "-v will create the Patch Tar file for the release .";
		echo_log "-m Module Name for the release .";
		echo_log "Module Name can be [FMSROOT/FMSTOOLS/FMSCLIENT]";
		echo_log "-h Help.";
}

#-----------------------------------------------------Functions End----------------------------------------------------------------------------#

#Main Starts Here ...........................

while getopts v:m:r:lh OPTION
do
		case ${OPTION} in
				v) VersionName="${OPTARG}" ;;
				m) ModuleName="${OPTARG}" ;;
				h) Help; exit 0;;
                \?)Help; exit 2;;
        esac
done;

Validations;
ExportSRCPaths;
CheckAndExtract;

#Main Ends Here ...........................
