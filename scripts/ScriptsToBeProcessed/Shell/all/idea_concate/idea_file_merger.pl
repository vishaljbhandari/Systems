#!/usr/bin/perl
use File::Basename ;
use POSIX qw(strftime);
use List::Util qw[min max];
use File::stat ;
use File::Basename ;
use IO::Handle;
sub Initialize
{
        $file = @_[0] ;
        $test_run = @_[1];
        #Added to Implement CR
        if (!( -f $file ))
        {
                die ( "\nConfiguration File : $file ,does not Exist! . Exiting !!! ");
        }     
        
        #End Of Change CR 
           open(config,$file) ;
        foreach $line(<config>)
        {
                chomp($line) ;
                if ($line =~ /^INPUTDIR/)
                {
                        $inputdir=(split(/=/,$line))[1] ;
                        if (-d $inputdir)
                        {
                                print LOGFILE "The directory " , $inputdir , " exists \n" ;
                        }
                        else
                        {
                                print LOGFILE "Warning!! The directory " , $inputdir , " does not exist \n" ;
                                exit ;
                        }
                }

                if ($line =~ /^OUTPUTDIR/)
                {
                        $outputdir=(split(/=/,$line))[1] ;
                        if (-d $outputdir)
                        {
                                print LOGFILE "The directory " , $outputdir , " exists \n" ;
                        }
                        else
                        {
                                print LOGFILE "Warning!! The directory " , $outputdir , " does not exist \n" ;
                                exit ;
                        }
                }

                if ($line =~ /^OUTPUT_FILE_SIZE/)
                {
                        $outputfilesize=(split(/=/,$line))[1] ;
                        if ($outputfilesize eq '')
                        {
                                print LOGFILE "The size of the output file size is not configured. Exiting !!!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The size of the output file size is configured to " ,$outputfilesize, "\n" ;
                        }
                }

                if ($line =~ /^WAIT_TIME/)
                {
                        $outputwaittime=(split(/=/,$line))[1] ;
                        if ($outputwaittime eq '')
                        {
                                print LOGFILE "The wait time for concatenating the file is not configured.  Exiting !!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The wait time for concatenating the file is configured to " ,$outputwaittime, "\n" ;
                        }
                }

                if ($line =~ /^DELIMETER/)
                {
                        $delimeter=(split(/=/,$line))[1] ;
                        if ($delimeter eq '')
                        {
                                print LOGFILE "The delimeter has not been configured . Exiting !!!! \n" ;
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The delimiter has been configured to '" ,$delimeter, "'\n" ;
                        }
                }

                if ($line =~ /^OUTPUT_FILE_PREFIX/)
                {
                        $outputfileprefix=(split(/=/,$line))[1] ;
                        if ($outputfileprefix eq '')
                        {
                                print LOGFILE "The output file prefix has not been configured. Exiting!!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The output file prefix has been configured to '" ,$outputfileprefix , "'\n" ;
                        }
                }

                if ($line =~ /^FILE_STABILITY_SECS/)
                {
                        $filestability=(split(/=/,$line))[1] ;
                        if ($filestability eq '')
                        {
                                print LOGFILE "The file stability has not been configured . Exiting !!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The file stability in seconds has been configured to '" ,$filestability , "'\n" ;
                        }
                }

                if ($line =~ /^RECORD_COUNT/)
                {
                        $recordcount=(split(/=/,$line))[1] ;
                        if ($recordcount eq '')
                        {
                                print "The record count has not been configured. Exiting !!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The record count has been configured to '" ,$recordcount , "'\n" ;
                        }
                }
	#Added for Record Config:
	if ($line =~ /^RECORD_CONFIG_TYPE/)
                {
                        $recordConfigType=(split(/=/,$line))[1] ;
                        if ($recordConfigType eq '')
                        {
                                print LOGFILE "The Record Config has not been configured . Exiting !!! \n";
                                exit ;
                        }
                        else
                        {
                                print LOGFILE "The Record Config has been configured to '" ,$recordConfigType , "'\n" ;
                        }
                }




        }
        $ConcatenationPrefix="consolidated_file_" . $outputfileprefix ;
        $SeqNum = 0 ;
                LOGFILE->autoflush;
}

sub SetOutputFileName {

        $currentTime = strftime("%Y%m%d%H%M%S", localtime);
                $ConsolidatedFileName = $ConcatenationPrefix . "_" . $currentTime .".txt" ;     
        #print " $ConsolidatedFileName ConsolidatedFileName $PreviousFileName ..... $SeqNum \n" ;
        if ($PreviousFileName ne $ConsolidatedFileName )        {
                $ConsolidatedFile= $ENV{'RANGERHOME'}."/tmp/".$ConsolidatedFileName ;
                $PreviousFileName = $ConsolidatedFileName ;
                #Added for CR :
                                #Append First Line of newly created ConsolidatedFile with "BEG"
                                open(BEG_PREFIX,  ">" , $ConsolidatedFile ) or die " BEG_PREFIX :: Cannot open the file  $ConsolidatedFile  $! ";
                                print BEG_PREFIX "BEG\n" ;
			#Appending recordType
				print BEG_PREFIX "RecordType:$recordConfigType\n"; 
			#End of change
                                close(BEG_PREFIX);
                                #End Of Change CR:
                                $SeqNum=0 ;
                $consolidateFileStartTime = time() ;
                $recordsWrittenInCurrentFile = 0 ;
        }
        else {
                $ConsolidatedFileName=$ConsolidatedFileName . "_" . $SeqNum ;
                $SeqNum = $SeqNum + 1 ;
                $ConsolidatedFile= $ENV{'RANGERHOME'}."/tmp/".$ConsolidatedFileName ;
                #Added for CR :
                                #Append First Line of newly created ConsolidatedFile with "BEG"
                                #print LOGFILE "Appending BEG to $ConsolidatedFile \n";
                                open(BEG_PREFIX,  ">" , $ConsolidatedFile ) or die " BEG_PREFIX :: Cannot open the file  $ConsolidatedFile  $! ";
                                print BEG_PREFIX "BEG\n" ;
				#Appending recordType
					print BEG_PREFIX "RecordType:$recordConfigType\n"; 
				#End of change
                                close(BEG_PREFIX);
                                #End Of Change CR : 
                                $consolidateFileStartTime = time() ;
                $recordsWrittenInCurrentFile = 0 ;
        }
}
sub AppendDataToFile {
        my ($fileName) = @_ ;
        open(OUTPUTFILEPREFIX,  ">>" , $ConsolidatedFile ) or die "Cannot open the file  $ConsolidatedFile  $! ";
        print LOGFILE $file  , "\n" ;
        open (FILE, $fileName) ;
        while (<FILE>)
        {               chomp($_);
                #Modified to Handle CR
                                if (($_=~ /^BEG/) ||  ($_=~ /^END/) || ($_=~/^RecordType/) )
                                {
                                        next;
                                }
                                #End of Change : CR     
                        #Modified to Implemet CR
                        #print  OUTPUTFILEPREFIX  "$_" . $delimeter . $filenameonly  . $delimeter . "$fileaccesstime \n" ;
						print  OUTPUTFILEPREFIX  "$_" . "\n" ;
                $recordsWrittenInCurrentFile += 1 ;
        }
        close (OUTPUTFILEPREFIX) ;
       unlink ($fileName) ;
}


sub ConsolidateFiles {
        #opendir(DIR, $inputdir) or die "Cannot open $inputdir $!";
        opendir(DIR, $inputdir) ;
                 #Sort a list of filenames by age (oldest last), efficiently.
        @OldFiles = (sort {-M $b <=> -M $a} <$inputdir/*>) ;
        closedir(DIR) ;
        $curtime = time() ;
        ##ADDED FOR CR 
        #IF array is empty and waittime has exceeded then append end and move file to uoutput directory.
        if (($#OldFiles == -1) && ((time() - $consolidateFileStartTime )>$outputwaittime ))
        {
                
                if ($recordsWrittenInCurrentFile > 0)
        {
        #print "\nWaited for more than wait time and recordsWrittenInCurrentFile[$recordsWrittenInCurrentFile]\n" ;
 
                #print "\nAPPENDING END TO :", $ConsolidatedFile ;
         #Appending "END" to the COnsolidated File
        open(END_PREFIX,  ">>" , $ConsolidatedFile ) or die "END_PREFIX :: Cannot open the file  $ConsolidatedFile  $! ";
        print END_PREFIX "END" ;
        close(END_PREFIX);
        rename($ConsolidatedFile, $outputdir."/".$ConsolidatedFileName)  or die "Cannot open $outputdir $!";
        print LOGFILE "File  $ConsolidatedFileName  moved to $outputdir  with $recordsWrittenInCurrentFile \n" ;
        #print  "File  $ConsolidatedFileName  moved to $outputdir  with $recordsWrittenInCurrentFile \n" ;
                LOGFILE->autoflush;
                SetOutputFileName () ; 
                
         }
        }
        
        ##END OF ADDITION


        foreach $file(@OldFiles)
        {
                if( -f $file)
                {
                        $filemtime = stat ($file)->mtime ;
                                                #print "$curtime" ,"$filemtime"; 
                        if (($curtime - $filemtime) > $filestability)
                        {                       
                                my $timeSinceFileOpened = (time() - $consolidateFileStartTime) ;

                                                                if (($recordsWrittenInCurrentFile == 0) ||
                                        ($recordsWrittenInCurrentFile > $recordcount ) ||
                                        ($timeSinceFileOpened > $outputwaittime))
                                {       
                                        if ($recordsWrittenInCurrentFile > 0)
                                        {
                                                                                #print "\nAPPENDING END TO :", $ConsolidatedFile ;
                                                                                #Added to Implement CR :
                                                                                #Appending "END" to the COnsolidated File 
                                                                                open(END_PREFIX,  ">>" , $ConsolidatedFile ) or die "END_PREFIX :: Cannot open the file  $ConsolidatedFile  $! ";
                                                                print END_PREFIX "END" ;
                                                        close(END_PREFIX);
                                                                                #End of CHange

                                                rename($ConsolidatedFile, $outputdir."/".$ConsolidatedFileName)  or die "Cannot open $outputdir $!";
                                                print LOGFILE "File  $ConsolidatedFileName  moved to $outputdir  with $recordsWrittenInCurrentFile \n" ;
                                        }
                                                                                SetOutputFileName () ;
                                        #print "\nConsolidatefileName :$ConsolidatedFile";
                                                                                print LOGFILE "Consolidating Data to file : $ConsolidatedFile \n " ;
                                }
                                $consolidatedFileSize = -s $ConsolidatedFile ;
                                if ($consolidatedFileSize >= $outputfilesize )
                                {
                                        if ($recordsWrittenInCurrentFile > 0)
                                        {
                                                rename($ConsolidatedFile, $outputdir."/".$ConsolidatedFileName)  or die "Cannot open $outputdir $!";
                                                print LOGFILE "File $ConsolidatedFileName moved to $outputdir with $recordsWrittenInCurrentFile \n" ;
                                        }
                                        SetOutputFileName () ;
                                        print LOGFILE "Consolidating Data to file : $ConsolidatedFile \n " ;
                                }

                                $fileaccesstime = strftime("%Y%m%d%H%M%S", localtime($filemtime));
                                $filenameonly = fileparse($file);
                                chomp($filenameonly);
                                AppendDataToFile($file) ;
                                print LOGFILE "Consolidated file : $file, Consolidated Record Count : $recordsWrittenInCurrentFile \n " ;
                        }
                }
        }
}

sub main
{
        $Today = strftime("%Y%m%d", localtime) ;#FORMAT : 20131004 <yyyymmdd>
        my $logfile = $ENV{'RANGERHOME'}."/LOG/Ideafilesconcatenator".$Today.'.log' ;   #PER DAY LOG FILE
        print "\nLOGFILE = ".$logfile."\n" ;
                open(LOGFILE, ">>",$ENV{'RANGERHOME'}."/LOG/Ideafilesconcatenator".$Today.'.log') or die "log opening faile $!" ;
        $StartTime = `date "+%D %H:%M:%S"`;
        print LOGFILE "################################################################################## \n";
        print LOGFILE "Opening log file $logfile \n " ;
        print LOGFILE "Start Time       :: $StartTime \n";
                LOGFILE->autoflush;
                Initialize (@ARGV) ;
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
