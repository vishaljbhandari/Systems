#!/bin/bash
TARGS=$#

if [ $TARGS -ne 1 ] ; then
    echo " USAGE : nohup $0 [Full data directory Path ]   >> $0.log 2>&1 "
       exit 2
fi

export src=$1
export CURDIR=`pwd`
export OFFLINE_CDR="/rocfm/subex_working_area/MANUAL_LOADING/CDR/DATA/process_INST_1"
export OFFLINE_GPRS_CDR="/rocfm/subex_working_area/MANUAL_LOADING/GPRS/DATA/process_INST_1"
export OFFLINE_RECHARGE="/rocfm/subex_working_area/MANUAL_LOADING/RECHARGE/DATA/process_INST_1"
export DBWRITER_ONLINE="/rocfm/NIKIRAROOT/FMSData/DBWriterData_ONLINE"

split() {
cd $src
for fileName in `ls -1rt success | head -15 `
 do
record=`head -1 $fileName`
recordType=`echo $record | cut -d" " -f1 | cut -d":" -f2`
batch=`echo $RANDOM`
            if [ $recordType == 1 ]
            then
				echo 'RecordType:1' > /tmp/recordtype
				perl -F"," -lane 'if ( $F[1] >= 100  && $F[1] <= 360  )  {print $_} ' $fileName >> $OFFLINE_CDR/$fileName\_$batch.tmp
				perl -F"," -lane 'if ( $F[1] < 100 && $F[1] != NULL )  {print $_} ' $fileName >> $DBWRITER_ONLINE/$fileName\_$batch.tmp
                if [ ! -s $OFFLINE_CDR/$fileName\_$batch.tmp ]
				then
					rm $OFFLINE_CDR/$fileName\_$batch.tmp 
				else
					cat /tmp/recordtype $OFFLINE_CDR/$fileName\_$batch.tmp >> $OFFLINE_CDR/$fileName\_$batch.dat
					touch $OFFLINE_CDR/success/$fileName\_$batch.dat
					rm $OFFLINE_CDR/$fileName\_$batch.tmp
				fi
				
				if [ ! -s $DBWRITER_ONLINE/$fileName\_$batch.tmp ] 
				then 
					rm $DBWRITER_ONLINE/$fileName\_$batch.tmp
				else
					cat /tmp/recordtype $DBWRITER_ONLINE/$fileName\_$batch.tmp >> $DBWRITER_ONLINE/$fileName\_$batch.dat
					touch $DBWRITER_ONLINE/success/$fileName\_$batch.dat
					rm $DBWRITER_ONLINE/$fileName\_$batch.tmp
				fi

				
            elif [ $recordType == 7 ]
            then
				 echo 'RecordType:7' > /tmp/recordtype
				 perl -F"," -lane 'if ( $F[1] >= 100  && $F[1] <= 360  )  {print $_} ' $fileName >> $OFFLINE_GPRS_CDR/$fileName\_$batch.tmp
			     perl -F"," -lane 'if ( $F[1] < 100 &&  $F[1] != NULL )  {print $_} ' $fileName >> $DBWRITER_ONLINE/$fileName\_$batch.tmp
                if [ ! -s $OFFLINE_GPRS_CDR/$fileName\_$batch.tmp ]
				then
					rm $OFFLINE_CDR/$fileName\_$batch.tmp 
				else
					cat /tmp/recordtype $OFFLINE_GPRS_CDR/$fileName\_$batch.tmp >> $OFFLINE_GPRS_CDR/$fileName\_$batch.dat
					touch $OFFLINE_GPRS_CDR/success/$fileName\_$batch.dat
					rm $OFFLINE_GPRS_CDR/$fileName\_$batch.tmp
				fi
				
				if [ ! -s $DBWRITER_ONLINE/$fileName\_$batch.tmp ] 
				then 
					rm $DBWRITER_ONLINE/$fileName\_$batch.tmp
				else
					cat /tmp/recordtype $DBWRITER_ONLINE/$fileName\_$batch.tmp >> $DBWRITER_ONLINE/$fileName\_$batch.dat
					touch $DBWRITER_ONLINE/success/$fileName\_$batch.dat
					rm $DBWRITER_ONLINE/$fileName\_$batch.tmp
				fi

            elif [ $recordType == 2 ]
            then
				 echo 'RecordType:2' > /tmp/recordtype
				 perl -F"," -lane 'if ( $F[1] >= 100  && $F[1] <= 360  )  {print $_} ' $fileName >> $OFFLINE_RECHARGE/$fileName\_$batch.tmp
			     perl -F"," -lane 'if ( $F[1] < 100 &&  $F[1] != NULL )  {print $_} ' $fileName >> $DBWRITER_ONLINE/$fileName\_$batch.tmp
                if [ ! -s $OFFLINE_RECHARGE/$fileName\_$batch.tmp ]
				then
					rm $OFFLINE_CDR/$fileName\_$batch.tmp 
				else
					cat /tmp/recordtype $OFFLINE_RECHARGE/$fileName\_$batch.tmp >> $OFFLINE_RECHARGE/$fileName\_$batch.dat 
					touch $OFFLINE_RECHARGE/success/$fileName\_$batch.dat
					rm $OFFLINE_RECHARGE/$fileName\_$batch.tmp
				fi
				
				if [ ! -s $DBWRITER_ONLINE/$fileName\_$batch.tmp ] 
				then 
					rm $DBWRITER_ONLINE/$fileName\_$batch.tmp
				else
					cat /tmp/recordtype $DBWRITER_ONLINE/$fileName\_$batch.tmp >> $DBWRITER_ONLINE/$fileName\_$batch.dat
					touch $DBWRITER_ONLINE/success/$fileName\_$batch.dat
					rm $DBWRITER_ONLINE/$fileName\_$batch.tmp
				fi


            else
			    cp $fileName $DBWRITER_ONLINE
				touch $DBWRITER_ONLINE/success/$fileName
			fi

		echo "[`date`] Processed file: $fileName   and BATCHNAME: $batch "
		#rm $fileName
		mv $fileName Processed
		rm success/$fileName
done
}
Main(){

while true; do

    #if [ "$(ls -A $src 2> /dev/null)" == "" ]; then
        sleep 30
        split
    #fi
done
}

Main

