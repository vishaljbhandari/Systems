#!/bin/bash
clear;
NBIT_HOME=`pwd`;
Installation_Time=`date +%Dr#%H:%M:%S`;
logFile=`date|awk '{ print "LOG/" $3 "_" $2 "_" $6 "_NBIT_Install.log" }'`
echo "logFile = $logFile"
String=".";

function echo_log()
{
    echo "$@"
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile
}

function log_only()
{
    echo "`date|awk '{ print $3 \"_\" $2 \"_\" $6 \" \" $4 }'` $@">>$NBIT_HOME/$logFile
}

function Display()
{
		string=$1;
		Format=$2;
		(( i = 0 )) ;
		Empty=".";
		while [ $i -lt $Format ]
		do
				string="$string$Empty" ;
				(( i = $i + 1 )) ;
		done;
		command="echo \"$string\"|awk '{print substr(\$0,0,$Format)'}>l;"
		echo $command>s.sh;
		sh s.sh;
		string=`cat l`
		String=`echo $string|perl -p -e 's#\.# #g'`
		rm l s.sh
}

function RollbackErrorCheck
{
		ELfile=$1;
		grep "^SP2" $ELfile 
		if [ $? -eq 0 ]
		then
			echo_log "SP Error occured in Rollback.Check the SQL Query syntax";	
			exit 111;
		fi;
		
		grep "^ERROR" $ELfile
		
		if [ $? -eq 0 ]
		then
			echo_log "SQL Error occured in Rollback.";	
			exit 111;
		fi;

}

function runMig()
{
		File_name=$1;
		install_type=$2;
		if [ ! -f MIGRATE/$File_name ]
		then
			echo_log " Migration File for this Release (MIGRATE/$File_name}) does not exist ";
			exit 4;
		fi;
		if [ "$DB_USER" == "" -o "$DB_PASSWORD" == "" ]
		then
			echo_log "DB_USER/DB_PASSWORD is not set";
			exit 4;
		fi;
		if [ "$install_type" == "UPGRADE" ]
		then
			>install.sql
				tagCounter=1
				blockName="INSTALL_$tagCounter";
				grep "$blockName" MIGRATE/$File_name>/dev/null 2>&1 ;
				status=$?;
				while [ $status -ne 1 ]
				do
					stLine=`grep -n "$blockName{" MIGRATE/$File_name|cut -d":" -f1`
					endLine=`grep -n "}$blockName" MIGRATE/$File_name|cut -d":" -f1`
					hd=` expr $endLine - 1` ;
					tl=` expr $endLine - $stLine - 1`;
					echo "whenever sqlerror exit $tagCounter;">>install.sql ;
					head -$hd MIGRATE/$File_name |tail -$tl >>install.sql;
					echo "commit;" >>install.sql;
					tagCounter=`expr $tagCounter + 1 `;
					blockName="INSTALL_$tagCounter";
					grep "$blockName" MIGRATE/$File_name>/dev/null 2>&1 ;
					status=$?;
				done;
				echo "exit 0" >>install.sql;
				sqlplus -s $DB_USER/$DB_PASSWORD @ install.sql >LOG/$File_name.log
				installStatus=$?;

				if [ $installStatus -eq 0 ]
				then
					grep "^SP2" LOG/$File_name.log;
					if [ $? -ne 0 ]
					then
						echo_log "Installation Completed Successfully";
						return 0;
					else
						echo_log "SP error occured.Check the SQL Query syntax";
					fi;	
				else	
					echo_log "The error is at block :$installStatus " ;
				fi


				echo_log "Rolling back the migration (Error Recovery)" ;
				>roll.sql
				tagCounter=$installStatus;
				blockName="ROLL_$tagCounter";
				while [ $tagCounter -gt 0 ]
				do
					#echo_log "the block number is $blockName" ;
					stLine=`grep -n "$blockName{" MIGRATE/$File_name|cut -d":" -f1`
					endLine=`grep -n "}$blockName" MIGRATE/$File_name|cut -d":" -f1`
					hd=` expr $endLine - 1` ;
					tl=` expr $endLine - $stLine - 1`;
					head -$hd MIGRATE/$File_name |tail -$tl >>roll.sql;
					tagCounter=`expr $tagCounter - 1 `;
					blockName="ROLL_$tagCounter";
				done;
				echo "exit 0" >>roll.sql;

				sqlplus -s $DB_USER/$DB_PASSWORD @ roll.sql >>LOG/${File_name}_Eroll.log
				RollbackErrorCheck "LOG/${File_name}_Eroll.log" ;
				echo_log "Roll Back Completed ";
				return 1;		
					
		else
				>roll.sql
				tagCounter=`grep "ROLL_.*{" MIGRATE/$File_name|wc -l|perl -p -e 's/ //g'`
				blockName="ROLL_$tagCounter";
				while [ $tagCounter -gt 0 ]
				do
					#echo_log "the block number is $blockName" ;
					stLine=`grep -n "$blockName{" MIGRATE/$File_name|cut -d":" -f1`
					endLine=`grep -n "}$blockName" MIGRATE/$File_name|cut -d":" -f1`
					hd=` expr $endLine - 1` ;
					tl=` expr $endLine - $stLine - 1`;
					head -$hd MIGRATE/$File_name |tail -$tl >>roll.sql;
					tagCounter=`expr $tagCounter - 1 `;
					blockName="ROLL_$tagCounter";
				done;
				echo "exit 0" >>roll.sql;
				ELfile="$File_name"
				sqlplus -s $DB_USER/$DB_PASSWORD @ roll.sql >LOG/${File_name}_roll.log
				RollbackErrorCheck "LOG/${File_name}_roll.log" ;
				echo_log "Version Roll Back Completed ";
				return 0;
		fi
		rm roll.sql install.sql
}


function GeneraTemplate
{
		echo_log "################################### Generating Migration Template ################################### ";

		if [ "$File_name" == ".db" ]
		then
				echo_log "please provide the Release name" ;
				exit 2;
		fi;

		if [ -f MIGRATE/$File_name ]
		then
				echo_log "Migration for this Release[${OPTARG}] is Available";
				exit 3;
		fi;
		echo_log "How Many blocks u want in Generated File [Default is 1] ";
		read n;

		if [ "$n" == "" ]
		then
			n=1;
		fi;
		k=1;
		>MIGRATE/$File_name;
		while [ $n -gt 1 -o $n -eq 1 ]
		do
				echo "#put your migration in this block">>MIGRATE/$File_name;
				echo "INSTALL_$k{">>MIGRATE/$File_name;
				echo "">>MIGRATE/$File_name;
				echo "}INSTALL_$k">>MIGRATE/$File_name;
				echo "#put your roll back statements for the above migration in this block.">>MIGRATE/$File_name;
				echo "ROLL_$k{">>MIGRATE/$File_name;
				echo "">>MIGRATE/$File_name;
				echo "}ROLL_$k">>MIGRATE/$File_name;
				echo "">>MIGRATE/$File_name;
				echo "">>MIGRATE/$File_name;
				((n=$n-1));
				((k=$k+1));
		done;		
				echo "#Multiple above kind of blocks can be created.">>MIGRATE/$File_name;
		echo_log "################################### Template file MIGRATE/$File_name generated ###################################";
}

function Help
{
		echo "Usage:  $PROG_NAME -[gv] ReleaseName ";
		echo "or	$PROG_NAME -[lh] ";
		echo "-g will create the migration template file";
		echo "-v will run the migration for particular release or the latest.";
		echo "-l will list the Current Migration version and Available Migration Versions.";
		echo "-h Help.";
		exit 0;
}

function DOInstall
{
		if [ ! -f .current_version.dbmig ]
		then
				LReleaseName="0_core";
				echo "$LReleaseName|$LOGNAME|$Installation_Time" >.current_version.dbmig
				echo "$LReleaseName|$LOGNAME|$Installation_Time" > .version.dbmig
		fi;
		
		CurrVerName=`cat .current_version.dbmig|cut -f1 -d"|"`;		
		CurrVerNo=`cat .current_version.dbmig|cut -f1 -d"|"|cut -f1 -d"_"`;		

		if [ "$VersionName" == "0" ]
		then
				VersionNo=0;
				VersionName="0_core";
		fi;		

		if [ ! -f MIGRATE/$VersionName.db -a "$VersionName" != "0_core" ] 
		then
				Install_Patch=`ls MIGRATE/|egrep "^${VersionName}_.*db|^0*${VersionName}_.*db" `;	
				if [ $? -ne 0 ]
				then
				echo_log "Required installation file MIGRATE/$VersionName.db  missing";
				exit 5;
				fi;
				VersionName=`echo $Install_Patch|cut -d "." -f1`;
		fi
		
		VersionNo=`echo $VersionName|cut -d "_" -f1`;
		VersionNo=`expr $VersionNo - 1 + 1`		
		
		if [ $? -eq 3 ]
		then
			echo_log "Version No should be integer: Make sure prefix(_) of Version name is integer";
			exit 999;
		fi;	

		File_name=$VersionName.db; 			

		if [ $CurrVerNo -eq $VersionNo ]
		then
			echo_log "Current Version and Release Version are Same ($CurrVerNo)";
			exit 666;
		fi;	

		if [ $CurrVerNo -gt $VersionNo ]
		then		
			echo_log "################################### Version Rolling back starts ############################";
			echo_log "Rolling back starts from $CurrVerName to $VersionName############################";
			Fline=`cat .version.dbmig|cut -f1 -d"|" |grep -n $CurrVerName|cut -d ":" -f1`
			Tline=`cat .version.dbmig|cut -f1 -d"|"|grep -n $VersionName|cut -d ":" -f1`
			
			if [ "$Fline" == "" -o "$Tline" == "" ]
			then
					echo_log "Required version($VersionName) not available for rollback"
					exit 5;
			fi

			hd=$Fline;
			tl=`expr $Fline - $Tline`;
			for i in `head -$hd .version.dbmig|cut -f1 -d"|"|tail -$tl|sort -nr`
			do
					echo_log "Rolling back the version : $i"
					runMig "$i.db" 'ROLL';
			done
			LReleaseName=$VersionName;
			echo "$LReleaseName|$LOGNAME|$Installation_Time" >.current_version.dbmig

		else
			echo_log "################################### Upgrading Starts #######################################";	
			grep "$VersionName" .version.dbmig >/dev/null 2>&1

			if [ $? -eq 1 ]
			then
					echo_log "This is newly added migration" ;
					runMig $File_name 'UPGRADE';
					status=$? 
					echo "the status is $status";
					if [ $status -eq 0 ]
					then
							LReleaseName=$VersionName;
							echo "$LReleaseName|$LOGNAME|$Installation_Time" >.current_version.dbmig
							echo "$LReleaseName|$LOGNAME|$Installation_Time" >> .version.dbmig
					fi;
					exit 0;
			fi;
			Fline=`cat .version.dbmig|cut -f1 -d"|" |grep -n $CurrVerName|cut -d ":" -f1`
			Tline=`cat .version.dbmig|cut -f1 -d"|" |grep -n $VersionName|cut -d ":" -f1`
			hd=`expr $Tline `;
			tl=`expr $Tline - $Fline `;
			for i in `head -$hd .version.dbmig|cut -f1 -d"|"|tail -$tl`
			do
					echo_log "Installing the version : $i ";
					runMig "$i.db" 'UPGRADE';
					status=$? 
					if [ $status -ne 0 ]
					then
						echo_log "The version $i has stopped Abrubtly ";
						exit 5;
					fi		
					echo "$i|$LOGNAME|$Installation_Time">.current_version.dbmig
			done

		fi;	

}

function List
{
		if [ ! -f .current_version.dbmig ]
			then
				echo_log "No Migration Installed.No versions available.";
				exit 777;
		fi;	

		FullText=`cat .current_version.dbmig`
		CrrVer=`echo $FullText |cut -d "|" -f1`
		CrrLogname=`echo $FullText |cut -d "|" -f2`
		CrrTime=`echo $FullText |cut -d "|" -f3`
		CurrVerNo=`echo $CrrVer|cut -f1 -d"_"`
		Current_version=$CrrVer;
		Current_versionNo=$CurrVerNo;

		echo "++++++++++++++++++++++++++++++++++++++++++++ ${ModuleName} VERSIONS +++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
		echo "Current Version :$CrrVer";
		echo "*******************************************************************************************************************";
		echo "* Version	ReleaseName		Mode 	InstalledBy		InstalledTime  			          *";	
		echo "*******************************************************************************************************************";

		for i in `cat  .version.dbmig`
		do
				CrrVer=`echo $i |cut -d "|" -f1`
				CrrLogname=`echo $i |cut -d "|" -f2`
				CrrDate=`echo $i |cut -d "|" -f3`
				CrrTime=`echo $i |cut -d "#" -f2`
				CurrVerNo=`echo $i|cut -f1 -d"_"`

				if [ $Current_versionNo -gt $CurrVerNo -o $Current_versionNo -eq $CurrVerNo ]
				then
					Mode="Active";
				else
					Mode="Rolled"
				fi;	

				
				Display "$CurrVerNo" 7;
				CurrVerNo=$String;
				Display "$CrrVer" 20;
				CrrVer=$String;
				Display "$CrrLogname" 20;
				CrrLogname=$String;
				Display "$CrrDate" 8;
				CrrDate=$String;
				Display "$CrrTime" 8;
				CrrTime=$String;
				
				
		echo "	$CurrVerNo $CrrVer   $Mode      $CrrLogname $CrrDate $CrrTime	"
		done;
		echo " "
		echo " "
		echo " "
		echo " "
		exit 0;		
}


#-----------------------------------------------------Functions End----------------------------------------------------------------------------#


# Main Starts Here..............................................

while getopts g:v:lh OPTION
do
		case ${OPTION} in
				g)File_name="${OPTARG}.db";GeneraTemplate;exit 0;;
				v)VersionName="${OPTARG}";DOInstall;;
				l) List ;exit 0;;
				h) Help;exit 0;; 
				\?)Help;exit 2;;
	    esac
		echo_log "################################### Migration Completed ####################################" ;
		exit 0;
done

echo "Try '$PROG_NAME -h' for more information.";

# Main Ends Here..............................................

