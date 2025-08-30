#!/usr/bin/perl
use File::Basename ;
use POSIX qw(strftime);
use List::Util qw[min max];
use File::stat ;
use File::Basename ;
use IO::Handle;


sub main
{
     	$currTime = strftime("%Y%m%d", localtime) ;#FORMAT : 20131004 <yyyymmdd>
		$currDir = `pwd`;
        my $logfile = $currDir."master-".$Today.'.log' ;   #PER DAY LOG FILE
        print "\nLOGFILE = ".$logfile."\n" ;
        open(LOGFILE, ">>", $logfile) or die "log opening faile $!" ;
        $StartTime = `date "+%D %H:%M:%S"`;
        print LOGFILE "################################################################################## \n";
        print LOGFILE "Opening log file $logfile \n " ;
        print LOGFILE "Start Time       :: $StartTime \n";
        LOGFILE->autoflush;
        Initialize (@ARGV) ;
       if($isOutPutFilePrefixSet eq  0)
		{
			print "isOutPutFilePrefixSet : $isOutPutFilePrefixSet";
			print LOGFILE "OUTPUT_FILE_PREFIX not Set\n";
		}
		 my $recordsWrittenInCurrentFile = 0 ;
        if ($test_run eq "999")
        {
                                    ConsolidateFiles ;
        }
        else
        {
                while (1)
                {
                        ConsolidateFiles ;
                                                sleep 10 ;
                }
        }
        $ENDTime = `date "+%D %H:%M:%S"`;
        print LOGFILE "\nEnd Time         :: $ENDTime\n";
        print LOGFILE "################################################################################## \n";
        close(LOGFILE);
}
main();
