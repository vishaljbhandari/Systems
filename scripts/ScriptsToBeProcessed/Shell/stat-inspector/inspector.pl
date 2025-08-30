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
					-LOG_FILE => "",
					-LEGACY_CONTINUITY_COUNT => 1
				);
					
$RunningConf{-LOG_FILE} = $CurrentWorkingDirectory."/log/inspector-".$RunningConf{-CURRENT_TIME}.".log";
					
%SelectionConf = (	
					-FILE_SELECTOR_REGEX => '\.h,|\.c,|\.cc,|\.cpp,',
					-FILE_DESELECTOR_REGEX => "\.java,",
					-FILE_LOCATION_TAG => "",
					-ERROR_LOOK_UP_PERIOD => 5
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
	
	%errors = ();
	%running_files = ();
	%running_days = ();
		
	%commit_summary = ();
	%commit_details = ();
	%commit_days = ();
	
	if (! -e $RunningConf{-CURRENT_DIR}.'/.InspectorRoot') {
		print "[ERROR] INVALID INSPECTOR DIRECTORY, Script Must Be Running From The Inspector Rood Directory\n";
	}

	$filename = $RunningConf{-CURRENT_DIR}.'/cases/.runDetails';
	if (! -e $filename) {
		print "[ERROR] Run details file is missing $filename\n";
	}

	my $last_run_date = `cat $filename | tail -1`;
	
	
	my $case_directory = $RunningConf{-CURRENT_DIR}."/cases/";
	print ("Cse Directory : $case_directory\n");
	my $last_case_directory = $RunningConf{-CURRENT_DIR}."/cases/C".$last_run_date;
	print ("Last Case Directory : $last_case_directory\n");
	print ("Last Run Date : $last_run_date\n");
	
	$stat_history = $RunningConf{-CURRENT_DIR}.'/cases/stat_history.db';
	if (! -e $stat_history) {
		print "[ERROR] Error Stat Hitory : $stat_history file missing\n";
		exit
	}
	print "[INFO] Error Stat Hitory : $stat_history file\n";
	
	open(my $data, '<', $stat_history) or die "[ERROR] Could not open $stat_history $!\n";
	
	while (my $line = <$data>) {
			chomp $line;
			my @fields = split "##" , $line;
			
			my $error_file = $fields[1];
			$errors{$error_file}{'FileName'} = $fields[4];
			$errors{$error_file}{'Project'} = $fields[10];
			$errors{$error_file}{'Module'} = $fields[11];
			
			my %error = {} ;
			
			$error{'Priority'} = $fields[5];
			$error{'Category'} = $fields[6];
			$error{'LineNumber'} = $fields[7];
			$error{'ErrorHash'} = $fields[8];
			$error{'FileHash'} = $fields[9];
			$error{'Link'} = $fields[12];
			$error{'ErrorVersion'} = 0;
			
			my $runday = $fields[0];
			my $err_id = $fields[3];
			
			$errors{$error_file}{"errors"}{$err_id}{int($runday)} = \%error;
			$running_days{int($runday)}{$err_id} = $error_file;
	}
	print ("\n\n >> >> >> All ERRORS >> >> >> >> \n");
	foreach my $dp_file (keys %errors){
		print ("\n\n[ >> ]  File: [$errors{$dp_file}{FileName}] $dp_file [$errors{$dp_file}{Project}:$errors{$dp_file}{Module}]\n");
		my $key = "errors";
		if($errors{$dp_file}{$key}){
			foreach my $err_id (keys $errors{$dp_file}{$key}){
				print ("\n  >>    Error Id [$err_id]\n");
				my $version = 1;
				foreach my $runday (sort {$a <=> $b} keys $errors{$dp_file}{$key}{$err_id}){
					$errors{$dp_file}{$key}{$err_id}{$runday}{ErrorVersion} = $version;
					print ("\n\t> Date $runday [V$errors{$dp_file}{$key}{$err_id}{$runday}{ErrorVersion}] | [$errors{$dp_file}{$key}{$err_id}{$runday}{Priority}:$errors{$dp_file}{$key}{$err_id}{$runday}{Category}]\n");
					print ("\t>   At $dp_file:$errors{$dp_file}{$key}{$err_id}{$runday}{LineNumber}\n");
					print ("\t>   Link $errors{$dp_file}{$key}{$err_id}{$runday}{Link}\n");
					print ("\t>   Hash [File] $errors{$dp_file}{$key}{$err_id}{$runday}{FileHash} [Error] $errors{$dp_file}{$key}{$err_id}{$runday}{ErrorHash}\n");
					$version++;
				}
			}
		}
	}
	
	print ("\n\n\n >> >> >> All RUN Day >> >> >> >> \n");
	foreach my $run_day (sort {$a <=> $b} keys %running_days){
		print ("\n\n[ >> ]  Running Days: $run_day\n");
		foreach my $err_id (keys $running_days{$run_day}){
			print ("  >>    Error Id [$err_id] : $running_days{$run_day}{$err_id}\n");
		}
	}
	
	print ("\n");
	$git_summary_history = $RunningConf{-CURRENT_DIR}.'/cases/git_summary_commits.txt';
	if (! -e $git_summary_history) {
		print "[ERROR] Error Git Summary Hitory : $git_summary_history file missing\n";
		exit;
	}
	print "[INFO] Git Summary Hitory : $git_summary_history file\n";
	
	open(my $gsdata, '<', $git_summary_history) or die "Could not open $git_summary_history $!\n";
	
	while (my $line = <$gsdata>) {
		chomp $line;
		my @fields = split "#-#" , $line;
					
		my %commit = {} ;
		my $exp_commit_id = $fields[1];
		$commit{'commit_id'} = $fields[0];
		$commit{'user_name'} = $fields[2];
		$commit{'user_id'} = $fields[3];
		$commit{'unix_date'} = $fields[5];
		$commit{'comment'} = $fields[6];
		$commit{'run_day'} = int($fields[4]);
		$run_Day = int($fields[4]);
		$commit_days{$run_Day}{$exp_commit_id} = $fields[3];
		$commit{'commit_date'} = localtime($fields[5]);
		
		$commit_summary{$exp_commit_id} = \%commit;
	}
	
	print ("\n");
	$git_detailed_history = $RunningConf{-CURRENT_DIR}.'/cases/git_detailed_commits.txt';
	if (! -e $git_detailed_history) {
		print "[ERROR] Error Git Detailed Hitory : $git_detailed_history file missing\n";
		exit;
	}
	print "[INFO] Git Detailed Hitory : $git_detailed_history file\n";
	
	open(my $gddata, '<', $git_detailed_history) or die "Could not open $git_detailed_history $!\n";
	
	while (my $line = <$gddata>) {
		chomp $line;
		my @fields = split "##" , $line;
					
		#my %commit = {} ;
		#$commit{'expended_commit_id'} = $fields[0];
		#$commit{'lines'} = $fields[2];
		#
		#my $file_path = $fields[1];
		#$commit_details{$file_path} = \%commit;
		
		$commit_details{$fields[1]}{$fields[0]} = $fields[2];
	}
	
	print ("\n");
	
	print ("\n-- Reporting --\n");
	
	my $start_date = int("20190723");
	my $end_date = int("20190802");
	
	my $first_run;
	my $last_run;
	
	$total_commits = 0;
	$count = 0;
	foreach my $run_day (sort {$a <=> $b} keys %running_days){
		if($run_day <= $start_date){
			$first_run = $run_day;
		}
		if($run_day <= $end_date){
			$last_run = $run_day;
		}
		my $err = keys $running_days{$run_day};
		print ("Run Day $run_day [Total Errors: $err ]\n");
		$total_commits = $total_commits + $err;
		$count++;
	}
	my $avg = $total_commits/$count;
	print ("Average Errors: $avg per run\n");
	print ("Start Date [$start_date] First Run [$first_run]\n");
	print ("End Date [$end_date] Last Run [$last_run]\n");
	$total_commits = 0;
	
	foreach my $run_day (sort {$a <=> $b} keys %commit_days){
		if($run_day >= $start_date && $run_day <= $last_run){
			$total_commits = $total_commits + (keys $commit_days{$run_day});
		}
	}
	print ("Total Commits in Duration $total_commits\n");
	
	# Print All Errors (in reverse order)
	
	print ("\n\n[>>] List Of All Errors(With Latest Versions)\n");
	print ("     ----------------------------------------\n");
	foreach my $dp_file (keys %errors){
		my $key = "errors";
		if($errors{$dp_file}{$key}){
			foreach my $err_id (keys $errors{$dp_file}{$key}){
				print (" >>  ErrId: [#$err_id][$errors{$dp_file}{FileName}] $dp_file [$errors{$dp_file}{Project}:$errors{$dp_file}{Module}]\n");
				$rsize = keys $errors{$dp_file}{$key}{$err_id};
				$error_type = "LEGACY";
				if($rsize <= int($RunningConf{-LEGACY_CONTINUITY_COUNT})){
					$error_type = "NEW";
				}elsif($rsize <= int($RunningConf{-LEGACY_CONTINUITY_COUNT})*2){
					$error_type = "ONGOING";
				} else {}
				$frun = (sort {$a <=> $b} keys $errors{$dp_file}{$key}{$err_id})[0];
				$lrun = (sort {$a <=> $b} keys $errors{$dp_file}{$key}{$err_id})[$rsize - 1];
				print ("     First Seen [$frun], Latest Seen [$lrun], Continuity [$rsize] Times Hence <[$error_type]> Error\n");
				print ("     Date $lrun [V-$errors{$dp_file}{$key}{$err_id}{$lrun}{ErrorVersion}] | [$errors{$dp_file}{$key}{$lrun}{$lrun}{Priority}:$errors{$dp_file}{$key}{$err_id}{$lrun}{Category}] At $dp_file:$errors{$dp_file}{$key}{$err_id}{$lrun}{LineNumber}\n");
				print ("     Link $errors{$dp_file}{$key}{$err_id}{$lrun}{Link}\n");
				print ("     Hash [File] $errors{$dp_file}{$key}{$err_id}{$lrun}{FileHash} [Error] $errors{$dp_file}{$key}{$err_id}{$lrun}{ErrorHash}\n");
				if($commit_details{$dp_file}){
					
					my @cid_arr_kays = sort keys $commit_details{$dp_file};
					my %commit_day_hash = {};
					foreach my $commit (@cid_arr_kays) { 
						$commit_day_hash{int($commit_summary{$commit}{'unix_date'})} = $commit;
					}
					my @cid_arr;
					
					foreach my $cdays (reverse sort keys %commit_day_hash) { 
						if($commit_day_hash{$cdays}) {
							push(@cid_arr, $commit_day_hash{$cdays}); 
						}
					}
					
					print ("	 >> Git Report - Total Commits ".(scalar @cid_arr)." On This File\n");
					print ("	 	First Instance [". $commit_summary{$cid_arr[(scalar @cid_arr) - 1]}{'commit_date'} ."] By ".$commit_summary{$cid_arr[(scalar @cid_arr) - 1]}{'user_name'}." [".$commit_summary{$cid_arr[(scalar @cid_arr) - 1]}{'user_id'}."] \n");
					print ("	 	>> Last 5 Commits, Reverse Chronologically, Lastest First\n");
					$rcount = 1;
					foreach my $rcid (@cid_arr){
						my $flag = "";
						if(int($start_date) < int($commit_summary{$rcid}{'run_day'}) && int($end_date) > int($commit_summary{$rcid}{'run_day'})) {
							print ("	 	[$rcount] ");
						} else {
							print ("	 	 $rcount  ");
						}
						print ($commit_summary{$rcid}{'commit_date'} ."] By ".$commit_summary{$rcid}{'user_name'}." [".$commit_summary{$rcid}{'user_id'}."][".int($commit_summary{$rcid}{'run_day'})."] [Lines#$commit_details{$dp_file}{$rcid}]\n");
						$rcount++;
						if($rcount <= 5){
							break;
						}
					}
					print ("	 	[x]Potential Error Owners\n");
				} else {
					print ("	 >> Git Report Not Available for File $dp_file\n");
				}
				print (" ---------------------------------------------------------------\n");
			}
		}
	}
	
#
#$running_days{int($runday)}{$err_id} = $error_file;
#
#
#
#foreach my $run_day (sort {$a <=> $b} keys %running_days){
#	my $total_errors = 0;
#	my $critical_error = 0;
#	my $low_errors = 0;
#	my $medium_error = 0;
#	my $high_errors = 0;
#	
#	my $v1_error = 0;
#	my $v2_error = 0;
#	my $v3_error = 0;
#	my $old_error = 0;
#	
#	my %v1_errors = {};
#	my %v2_errors = {};
#	my %v3_errors = {};
#	my %old_errors = {};
#	
#	my $error_id = running_days
#	
#
#
#}
	
	
	
	my $report_content = $RunningConf{-CURRENT_DIR}."/dp_report_inter";	
	open(FH, '>', $report_content) or die "Could not open $report_content $!\n";
	
	
	
	#print FH $str;
 
	close(FH);
}

Main();
