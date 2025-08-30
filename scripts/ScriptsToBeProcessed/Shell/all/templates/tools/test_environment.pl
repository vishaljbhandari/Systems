#!/bin/perl
use File::stat ;
use File::Basename ;
use File::Copy ;
use FileHandle;
use Cwd qw();

my $curr_dir;
my $var_list_file;

sub main
{
	$curr_dir=Cwd::abs_path();
	open(LOGFILE, ">>", "$curr_dir/logs/release_installer.log") or return 16 ;
	if (! -f "$curr_dir/.inside_lock")
	{
		print LOGFILE "[ERROR][TEST_ENVIRONMENT] Tool Not Running From Release Directory, Running from ($curr_dir/.inside_lock)\n";
		exit 10;
	}
	$var_list_file = @_[0];	
	if (! -f "$var_list_file")
        {
		print LOGFILE "[ERROR][TEST_ENVIRONMENT] Tool is running with missing/invalid Argument File ($var_list_file)\n";
                print "Argument File($var_list_file) Is Missing\n";
                exit 11;
        }
	if (! -d "$curr_dir/tmp")
	{
		unless(mkdir "$curr_dir/tmp"){
			exit 12;
		}
	}
	open(LOGFILE, ">>", "$curr_dir/logs/release_installer.log") or return 16 ;
	open(TMPFILE, ">", "$curr_dir/tmp/test_environment") or return 15 ;
	print LOGFILE "[ERROR][TEST_ENVIRONMENT] Creating Temporary Enviroment Check Script\n";
	print TMPFILE '#!/bin/bash';
	print TMPFILE 'LOG_FILE='.$curr_dir.'/logs/release_installer.log';
	print TMPFILE 'ev="Environmental Variable"';
	
        printf "\t%s\n" "[INFO] Checking $ev"
        e="[DB_HOSTNAME] $ev is ";
        if [ -z $"DB_HOSTNAME" ]; then echo -e "[ERROR] $e not set" >> $LOG_FILE; printf "\t%s\n" "[ERROR] $e is not set"; exit 77;
        else echo -e "[INFO] $e set to [$DB_HOSTNAME]" >> $LOG_FILE; printf "\t%s\n" "$e set to [$DB_HOSTNAME]"; fi

        e="[DB_PASSWORD] $ev is ";
        if [ -z $"DB_PASSWORD" ]; then echo -e "[ERROR] $e not set" >> $LOG_FILE; printf "\t%s\n" "[ERROR] $e is not set"; exit 77;
        else echo -e "[INFO] $e set to [$DB_PASSWORD]" >> $LOG_FILE; printf "\t%s\n" "[INFO] $e set to [$DB_PASSWORD]"; fi	

	
}


main(@ARGV);
