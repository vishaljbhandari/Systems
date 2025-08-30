#! /bin/bash

# Customize below variables
TotalDirs=2
GroupDirs=2
InputDir=/home/vishalb/nedjma_subscriber_splitter/in/
OutputDir=/home/vishalb/nedjma_subscriber_splitter/out/
LogFile="subhashsplitter.log"

GetConfig()
{
    if [ ! -d $InputDir ]
    then
        echo "Error : Parent Directory Does Not Exists"
        exit 1
    fi

    mkdir -p $InputDir/RejectedHashSplitterFiles/

    for (( i = 1; i <= $TotalDirs; ++i ))
    do
        TempDir=$OutputDir"_"$i

        if [ ! -d $TempDir ]
        then
            echo "Creating Directory : $TempDir"

            mkdir -p $TempDir/Process
            mkdir -p $TempDir/Process/success
            mkdir -p $TempDir/Processed
            mkdir -p $TempDir/Rejected
            mkdir -p $TempDir/LicenseRejected
            mkdir -p $TempDir/FutureRecordsRejected
        fi
    done
}

ReadAndSplitFiles()
{
    for file in `ls -tr $InputDir/Process/success`
    do
        dataFile=$InputDir/Process/$file
        header_data=`head -1 $InputDir/Process/$file`

        HashFieldPosition=1
        Delimiter="|"
        GroupBase=1

        TotRec=`cat $dataFile | wc -l | tr -d ' '`
        let TotRec=$TotRec-1

        hash_cnt=0

        #echo "Splitting $dataFile ($TotRec recs)..." >>$LogFile

        gawk -v header="$header_data" -v file_name="$file" \
             -v path="$OutputDir" -v hash_max="$GroupDirs" \
             -v field_pos="$HashFieldPosition" -v next_field_pos="$HashFieldPosition" \
             -v delimiter="$Delimiter" -v base="$GroupBase" \
             'BEGIN {\
                    FS = delimiter
                    OFS = delimiter
                    for ( i = 0; i <hash_max; i++ )
                    {
                        base_offset = base + i
                        record_count[base_offset] = 0
                        print header > (path "_" base_offset "/Process/" file_name "_" base_offset)
                    }
                }

                {
                    phone_number = $field_pos ;

                    if ( phone_number == "" )
                    {
                        phone_number = $next_field_pos ;
                    }

                    if (phone_number != "" && NR != 1)
                    {
                        gsub(/[[:space:]]*/,"",phone_number) ;
                        hash_ph = strtonum(substr(phone_number, length(phone_number)-3, length(phone_number))) ;
                        hash_ph = int(((hash_ph/1000) % 10) + ((hash_ph/100) % 10) + ((hash_ph/10) % 10) + (hash_ph % 10)) % hash_max ;

                        base_offset = base + hash_ph
                        record_count[base_offset] += 1
                        print > (path "_" base_offset "/Process/" file_name "_" base_offset)
                    }
                }

                END {\
                    for ( i = 0; i <hash_max; i++ )
                    {
                        base_offset = base + i

                        if ( record_count[base_offset] != 0 )
                        {
                            print "" > (path "_" base_offset "/Process/success/" file_name "_" base_offset)
                        }
                        else
                        {
                            system ("rm -f " path "_" base_offset "/Process/" file_name "_" base_offset)
                        }
                    }
                }' $dataFile

        status=$?

        if [ $status -eq 0 ]
        then
            rm -f $InputDir/Process/$file
            rm -f $InputDir/Process/success/$file
        else
            #echo "Failed to split file $dataFile. Moved to RejectedHashSplitterFiles" >>$LogFile
            mv $InputDir/Process/$file $InputDir/RejectedHashSplitterFiles/
            rm -f $InputDir/Process/success/$file
        fi
    done
}

main()
{
    GetConfig

    while true
    do
        ReadAndSplitFiles
        sleep 1
    done
}

main $*
