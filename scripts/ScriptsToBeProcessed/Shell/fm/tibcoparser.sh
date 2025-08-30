## This parser has been written by SAL to convert the XML files from TIBCO interface to flat files before feeding
## the subscriber activation information to the Subscriber Data adapter
## Logic :

## All the files that are present under the location $RANGERHOME/TIBCO will be processed by the parser
## The XML files will be checked for the CONTRACT_ID information, else will be rejected and logged
## under the $RANGERHOME/RangerRoot/LOG folder.
## The Services information and the status will be spooled separately and this will be compared with a
## predefined file (spooled from the Table DU_SERVICE_TIBCO) and accordingly the status will be decoded.
## Please note incase of any new addition to the services information then the same has to be updated in
## the Table DU_SERVICE_TIBCO and the parser need to be restarted.

# ! /bin/bash


. $RANGERHOME/sbin/rangerenv.sh 

function date_conversion ()
{
dao=`grep -w DATE_OF_ACTIVATION $file |cut -c 21-30`
date_of_act=`echo $dao |cut -c4,5`"/"`echo $dao |cut -c1,2`"/"`echo $dao |cut -c7-10`
date_of_birth=`grep -w DATE_OF_BIRTH $file |cut -c 16-25`
dob=`echo $date_of_birth |cut -c4,5`"/"`echo $date_of_birth |cut -c1,2`"/"`echo $date_of_birth |cut -c7-10`

}


function unf_conversion ()

{
grep -w $i $file |tr -d  "/" |tr -s " "|perl -pi -e 's/ //'| sed -e 's/<[^<>]*>//g' > temporary

cat temporary |grep ^""$ > /dev/null 2>&1
if [ $? = 1 ]
 then
cat temporary |grep ^+971 > /dev/null 2>&1
   if [ $? = 1 ]
    then 
    cat temporary |grep ^971 > /dev/null 2>&1
      if [ $? = 1 ]
       then 
        contact_no="+971"`cat temporary|cut -d " " -f1`
      else
        contact_no="+"`cat temporary|cut -d " " -f1`
      fi
      else
      contact_no=`cat temporary |cut -d " " -f1`
    fi
    else
    contact_no=`cat temporary|cut -d " " -f1`
  fi
  
}  

function service_mapping()
{

 rm status_check > /dev/null 2>&1
 rm status_check_serv > /dev/null 2>&1
 rm status_merge.tmp > /dev/null 2>&1

 grep -w SN_CODE $file > status_check
 grep -w SN_STATUS $file > status_check_serv

#sdiff status_check status_check_serv | tr -s " " |cut -d " " -f1,3 |sed -e 's/<[^<>]*>//g' | sort -n |perl -pi -e 's/deactive/1/g' | perl -pi -e 's/active/0/g'| perl -pi -e 's/on-hold/1/g'| perl -pi -e 's/suspended/1/g' > status_merge.tmp
sdiff status_check status_check_serv | tr "\t"  " "|tr -s " "|cut -d " " -f1,3|sed -e 's/<[^<>]*>//g' | sort -n |perl -pi -e 's/deactive/1/g' | perl -pi -e 's/active/0/g'| perl -pi -e 's/on-hold/1/g'| perl -pi -e 's/suspended/1/g'>status_merge.tmp

}

function db_table_service()

{

rm service_contract.dat > /dev/null 2>&1
rm compare_file.dat > /dev/null 2>&1
rm compare_file > /dev/null 2>&1

sqlplus -s /nolog << EOF > /dev/null 
CONNECT_TO_SQL
whenever sqlerror exit 5 ;
set heading off
set wrap off
set newpage none
set termout off
set trimspool on
spool compare_file.dat
select ID from DU_SERVICE_TIBCO;
spool off
/
EOF
grep -v "SQL" compare_file.dat |grep -v "row" > compare_file
for i in `cat compare_file`; do grep -w $i status_merge.tmp >>service_contract.dat ; if [ $? = 1 ]; then  echo $i "1" >> service_contract.dat;fi; done
final_services=`cat service_contract.dat|perl -pi -e 's/\n/ /g'`
}

function identification_document ()

{

rm document_type > /dev/null 2>&1
rm document_number > /dev/null 2>&1
rm document_temp > /dev/null 2>&1


grep -w DOCUMENT_TYPE $file > document_type
grep -w DOCUMENT_NUMBER $file > document_number

#sdiff document_type document_number |tr -s " " |cut -d " " -f1,3 |sed -e 's/<[^<>]*>//g' > document_temp
sdiff document_type document_number |tr "\t" " "|tr -s " "|cut -d " " -f1,3|sed -e 's/<[^<>]*>//g' > document_temp
company=`grep Company document_temp | cut -d " " -f2`
passport=`grep Passport document_temp| cut -d " " -f2`
license=`grep License document_temp| cut -d " " -f2`

}
function phone_no_unf()
{
cd $RANGERHOME/RangerData/temp

echo "CONTACT_PHONE_NUMBER" > phn_unf
echo "OFFICE_PHONE_NUMBER" >> phn_unf
echo "PHONE_NUMBER"        >> phn_unf
echo "HOME_PHONE_NUMBER"   >> phn_unf

for i in `cat phn_unf`
do
case $i in

CONTACT_PHONE_NUMBER)
unf_conversion
#echo $contact_no
contact_number=$contact_no;;

OFFICE_PHONE_NUMBER)
unf_conversion
#echo $contact_no
office_number=$contact_no;;

PHONE_NUMBER)
unf_conversion
#echo $contact_no
phone_number=$contact_no;;

HOME_PHONE_NUMBER)
unf_conversion
#echo $contact_no
home_number=$contact_no;;

esac
done
}

function tibco_parser_exe()
{

cd $RANGERHOME/RangerData/temp

 rm initial_data > /dev/null 2>&1
 rm next_stage > /dev/null 2>&1


grep -w ACCOUNT_ID $file |tr -d  "/"|  tr -s " " | uniq >> initial_data
grep -w FIRST_NAME $file |tr -d  "/"|  tr -s " " >> initial_data
grep -w MIDDLE_NAME $file |tr -d  "/"|  tr -s " " >> initial_data
grep -w LAST_NAME $file |tr -d  "/"|  tr -s " " >> initial_data
grep -w ADDRESS_LINE1 $file |tr -d  "/"|  tr -s " " >> initial_data
grep -w ADDRESS_LINE2 $file |tr -d  "/"|  tr -s " " >> initial_data
grep -w ADDRESS_LINE3 $file |tr -d  "/"|  tr -s " " >> initial_data
grep -w CITY $file |tr -d  "/"|  tr -s " " >> initial_data
grep -w POST_CODE $file |tr -d  "/"|  tr -s " " >> initial_data
##grep -w HOME_PHONE_NUMBER $file |tr -d  "/"|  tr -s " " >> initial_data
##grep -w OFFICE_PHONE_NUMBER $file |tr -d  "/"|  tr -s " " >> initial_data
##grep -w CONTACT_PHONE_NUMBER $file |tr -d  "/"|  tr -s " " >> initial_data
##grep -w CONTACT_PHONE_NUMBER $file |tr -d  "/"|  tr -s " " >> initial_data ## MCN1 ## 
##grep -w OFFICE_PHONE_NUMBER $file |tr -d  "/"|  tr -s " "  >> initial_data ## MCN2 ## 
echo "<"HOME_PHONE_NUMBER">"$home_number"<"HOME_PHONE_NUMBER">" >> initial_data
echo "<"OFFICE_PHONE_NUMBER">"$office_number"<"OFFICE_PHONE_NUMBER">" >> initial_data
echo "<"CONTACT_PHONE_NUMBER">"$contact_number"<"CONTACT_PHONE_NUMBER">" >> initial_data
echo "<"CONTACT_PHONE_NUMBER">"$home_number"<"CONTACT_PHONE_NUMBER">" >> initial_data
echo "<"OFFICE_PHONE_NUMBER">"$office_number"<"OFFICE_PHONE_NUMBER">" >> initial_data
grep -w GROUP_NAME $file |tr -d  "/"|  tr -s " " >> initial_data ## The Group Name needs to be checked in the input record 
grep -w BILL_CYCLE $file |tr -d  "/"|  tr -s " "| tr -d '[a-z]'| tr -d '[A-Z]' >> initial_data ## The Bill Cycle needs to be checked in the input record 
echo  "<CREDIT_EXPIRY_DATE>" >> initial_data
##grep -w DATE_OF_ACTIVATION $file |tr -d  "/"|  tr -s " " >> initial_data
##grep -w DATE_OF_ACTIVATION $file |cut -c 21-30| tr -s " " >> initial_data
echo "<"DATE_OF_ACTIVATION">"$date_of_act"<"DATE_OF_ACTIVATION">" >> initial_data
##echo "< >""+""< >" >> initial_data
##grep -w PHONE_NUMBER $file |tr -d  "/"|  tr -s " " >> initial_data
echo "<"PHONE_NUMBER">"$phone_number"<"PHONE_NUMBER">" >> initial_data
grep -w CO_STATUS $file |perl -pi -e 's/Active/a/g'|perl -pi -e 's/Deactive/d/g'|perl -pi -e 's/Suspend/s/g'|tr -d "/"|tr -s " " >> initial_data
grep -w IMSI $file |tr -d  "/"|  tr -s " " >> initial_data
echo  "<IMEI>" >> initial_data
echo "<"SERVICE">"$final_services"<"SERVICE">" >> initial_data
grep -w RATE_PLAN $file |tr -d  "/"|  tr -s " " >> initial_data
echo "<"SUBSCRIPTION_TYPE">"GSM"<"SUBSCRIPTION_TYPE">" >> initial_data
##grep -w DATE_OF_BIRTH $file |tr -d  "/"|  tr -s " " >> initial_data
echo "<"DATE_OF_BIRTH">"$dob"<"DATE_OF_BIRTH">">> initial_data
echo  "<CREDIT_CLASS>" >> initial_data
echo  "<END_SERVICE_DATE>" >> initial_data
grep -w CREDIT_LIMIT $file |tr -d  "/"|  tr -s " " >> initial_data
grep -w NATIONALITY $file |tr -d  "/"|  tr -s " " >> initial_data
echo "<"PASSPORT_NUMBER">"$passport"<"PASSPORT_NUMBER">" >> initial_data
echo "<"COMPANY_REGISTRATION_NUMBER">"$company"<"COMPANY_REGISTRATION_NUMBER">" >> initial_data
grep -w CONTRACT_ID $file |tr -d  "/"|  tr -s " " >> initial_data
echo "<"LICENSE">"$license"<"LICENSE">" >> initial_data
echo  "<OPT_FIELD_1>" >> initial_data
echo  "<OPT_FIELD_2>" >> initial_data
echo  "<OPT_FIELD_3>" >> initial_data
echo  "<OPT_FIELD_4>" >> initial_data
echo  "<OPT_FIELD_5>" >> initial_data
echo  "<OPT_FIELD_6>" >> initial_data
echo  "<OPT_FIELD_7>" >> initial_data
echo  "<OPT_FIELD_8>">> initial_data
##echo  " <OPT_FIELD_9>|" >> initial_data
##echo  " <DUMMY> " >> initial_data

  ##cat initial_data | perl -pi -e 's/ //'| perl -pi -e 's/\n//g' |perl -pi -e 's/> </>|</g' > next_stage
  ##sed -e 's/<[^<>]*>//g' next_stage |perl -pi -e 's/$/\n/g'> next_stage.tmp
  ##cat initial_data | perl -pi -e 's/ //'| perl -pi -e 's/\n/|/g' >next_stage


cat initial_data  | perl -pi -e 's/\n/|/g' > next_stage
cat next_stage | perl -pi -e 's/<[^<>]*>//g'|perl -pi -e 's/$/\n/g'|perl -pi -e 's/^/|/g' > next_stage.tmp

  mv next_stage.tmp $RANGER5.4HOME/RangerData/DataSourceSubscriberData/$file
  touch $RANGER5.4HOME/RangerData/DataSourceSubscriberData/success/$file
  rm $file > /dev/null 2>&1
  
}

## -- Main Program

##. $RANGERHOME/sbin/rangerenv.sh

#DB_USER=ranger
#DB_PASSWORD=ranger
#TIBCO_LOG=/ranger/ranger/RangerRoot/LOG
TIBCO_LOG=$RANGERHOME/LOG
#RANGERHOME=/ranger/ranger/RangerRoot
LOCATION=$RANGER5.4HOME/RangerData/TIBCO
##TIBCO_EXE_LOC=/ranger/ranger/subex_working_area/du/tibco_interface
md=`date +"%m%d%H%M%S"`

while [ 1 ]
  do
   cd $LOCATION/success
   COUNT=`ls |wc -l`
    if [ $COUNT != 0 ]
     then
      for file in `ls`
       do
       cd $LOCATION/success
       rm $file
       cd $LOCATION
       grep -w CONTRACT_ID $file > /dev/null 2>&1
        if [ $? = 0 ]
         then 
         echo " File Processed : $file " >> $TIBCO_LOG/tibco.log
         cp $file $RANGERHOME/RangerData/temp
         rm $file
         cd $RANGERHOME/RangerData/temp
        cat $file | perl -pi -e 's/></>\n</g' > "$file".tmp
        mv "$file".tmp $file




	service_mapping
        db_table_service
        identification_document
        phone_no_unf
        date_conversion
        tibco_parser_exe

        else 
        echo " File Rejected : $file  Phone Number details not found " >> $TIBCO_LOG/tibco.log
        rm $file
        fi
  	sleep 18
      done
    fi
done

