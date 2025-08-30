#!/usr/bin/perl
use Cwd;
my $dir = getcwd;

sub ConfigArgsProcessing {
	my %configFileParamHash = ();
	open ( _FH, $configFileName ) or die "[ERROR] Unable to open config file: $!";
	 
	while ( <_FH> ) {
		chomp;
		s/#.*//;                # ignore comments
		s/^\s+//;               # trim leading spaces if any
		s/\s+$//;               # trim leading spaces if any
		next unless length;
		my ($_configParam, $_paramValue) = split(/\s*=\s*/, $_, 2);
		$configFileParamHash{$_configParam} = $_paramValue;
	}
	close _FH;
}


sub ConfigProcessing {
$CurrentWorkingDirectory = getcwd;;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime();
%Configs = (	
					-SCRIPT_NAME => $ScriptName, 
					-STAT_DATA_FILE_PREFIX => "stat_report_file_",
					-STAT_DATA_FILE_SUFFIX => ".txt"
					-STAT_DATABASE_FILE => $configFileParamHash{ERROR_DATABASE_FILE}, 
					-MASTER_DATA_FILE => "stat_master.data",
					-STAT_DATA_FILE => "stat_report.csv"
			);

%RunningConf = ( 	
                    -CURRENT_DATE => sprintf( "%04d%02d%02d", $year+1900, $mon+1, $mday),
					-CURRENT_TIME => sprintf( "%04d%02d%02d%02d%02d%02d", $year+1900, $mon+1, $mday, $hour, $min, $sec),
					-CURRENT_DIR => $CurrentWorkingDirectory,
					-LOG_DIR => $CurrentWorkingDirectory."/log",
					-LOG_FILE => ""
				);
					
$RunningConf{-LOG_FILE} = $CurrentWorkingDirectory."/log/inspector-".$RunningConf{-CURRENT_TIME}.".log";
					
%SelectionConf = (	
					-FILE_SELECTOR_REGEX => '\.h,|\.c,|\.cc,|\.cpp,',
					-FILE_DESELECTOR_REGEX => "\.java,",
					-FILE_LOCATION_TAG => ""
				);
				

%GitConfigs	=	(
					-SOURCE_ROOT_DIRECTORY => $configFileParamHash{GIT_SOURCE_ROOT},
					-LOCAL_GIT_DIRECTORY => "git"
				);


%StatConfigs	=	(
					-SOURCE_ROOT_DIRECTORY => "",
					-LOCAL_GIT_DIRECTORY => "git"
				);
}

sub Main(){
	$ScriptName = $0;
	$NumArgs = $#ARGV + 1;
	if ($NumArgs != 1) {
		print "[Error] Usage: $ScriptName config_file\n";
		exit 999;
	}

	$configFileName = $ARGV[0];
	print ("[ Info] Config file $configFileName\n");
	ConfigArgsProcessing();
	ConfigProcessing();
	print ("[ Info] Running directory is $RunningConf{-CURRENT_DIR}\n");
	
	if (! -e $RunningConf{-CURRENT_DIR}.'/.InspectorRoot') {
		print "[ERROR] INVALID INSPECTOR DIRECTORY, Script Must Be Running From The Inspector Rood Directory\n";
	}

	$filename = $RunningConf{-CURRENT_DIR}.'/cases/.runDetails';
	if (! -e $filename) {
		print "[ERROR] Run details file is missing $filename\n";
	}

	my $last_run_date = `grep -v '^\$' \$filename | tail -1`;
	my $case_directory = $RunningConf{-CURRENT_DIR}."/cases/";
	print ("case_directory : ".$case_directory);
	my $last_case_directory = $RunningConf{-CURRENT_DIR}."/cases/C".$last_run_date;
	print ("last_case_directory : ".$last_case_directory);
	
	
	$stat_history = $RunningConf{-CURRENT_DIR}.'/cases/stat_history.db';
	if (! -e $stat_history) {
		print "[ERROR] Error Stat Hitory : $stat_history file missing\n";
	}
	print "[INFO] Error Stat Hitory : $stat_history file\n";
	
	open(my $data, '<', $stat_history) or die "Could not open '$stat_history' $!\n";
	my $first = 0;
	while (my $line = <$data>) {
		if($first > 0) {
			chomp $line;
			my @fields = split "##" , $line;
			$errors{$fields[2]}{'FilePath'} = $fields[3];
			$errors{$fields[2]}{'Project'} = $fields[10];
			$errors{$fields[2]}{'Module'} = $fields[11];
			
			my %error ;
			
			$error{'Date'} = $fields[0];
			$error{'Priority'} = $fields[4];
			$error{'Category'} = $fields[5];
			$error{'LineNumber'} = $fields[6];
			$error{'ErrorVersion'} = $fields[7];
			$error{'5LineErrorHashCode'} = $fields[8];
			$error{'FileHash'} = $fields[9];
			$error{'Link'} = $fields[12];
			
			$errors{$fields[2]}{"errors"}{$fields[1]} = \%error;
		}
		$first++;
	}
	
	
}

Main();