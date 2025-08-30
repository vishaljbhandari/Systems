#!/bin/perl
use File::stat ;
use File::Basename ;
use File::Copy ;
use FileHandle;

sub ReadConfigurationsFile{
	my ($file) = @_ ;
	return 0 if ( ! -f $file);

	my $validator = `egrep -i '^RecordType:$record_config_id' $fileName | wc -l`;
	return -2 if ( $validator == 0 ) ; 
	
	my $datestring = sprintf("%u", (strftime "%Y%m%d%H%M%S", localtime));
	my @array = `egrep -vi 'RecordType|\^\$' $fileName | gawk -F$delimiter '{input[\$$file_name_field_location]++; network[\$$file_name_field_location]=\$$Network_field_position}END{for (file in input) print file"##"$datestring"#"input[file]"#"input[file]"#0#"network[file]}'`;
	#E2E_RAT_F_NAME, E2E_RAT_C_TIME, E2E_RAT_A_TIME, E2E_RAT_I_RECS, E2E_RAT_O_RECS, E2E_RAT_F_RECS, E2E_RAT_N_ID
	my $arr_size = 0;
	my $records_used = 0;
	foreach my $Summerizefile (@array) {
		push (@DBInserts, $Summerizefile);
		#print LOGFILE "Summerizefile Record $Summerizefile\n";
		$records_used = $records_used + MakeDBInserts();
		$arr_size++;
	} 
	$TotalRecords = `egrep -vi 'RecordType|\^\$' $fileName | wc -l`;
	$TotalRecords =~ s/\s+$//;
	print LOGFILE "Total Summarized Entries[$arr_size], DB Entries[$records_used] , File[".basename($fileName)."] with Records[$TotalRecords]\n";
	return $arr_size - $records_used;
}

sub main 
{
	my $logfile = $ENV{'RANGERHOME'}."/LOG/IdeaGPRSFilesSummerizer".$Today.'.log' ;
	print "\nLOGFILE = ".$logfile."\n" ;
	open(LOGFILE, ">>",$ENV{'RANGERHOME'}."/LOG/IdeaGPRSFilesSummerizer".$Today.'.log') or die "log opening faile $!" ;
	$StartTime = strftime("%Y/%m/%d %T", localtime(time())) ;
	print LOGFILE "Opening log file $logfile \n " ;
	print LOGFILE "################################################################################## \n"; 
	print LOGFILE "Start Time       :: $StartTime \n";
	LOGFILE->autoflush;
	Initialize (@ARGV) ;

	SummerizingFiles ;

	$ENDTime = strftime("%Y/%m/%d %T", localtime(time())) ;
	print LOGFILE "\nEnd Time         :: $ENDTime\n";
	print LOGFILE "################################################################################## \n"; 
	close(LOGFILE);
}
main();
