#! /bin/bash
#################################################################################
#  Copyright (c) Subex Azure Limited 2006. All rights reserved.                 #
#  The copyright to the computer program (s) herein is the property of Subex    #
#  Azure Limited. The program (s) may be used and/or copied with the written    #
#  permission from Subex Systems Limited or in accordance with the terms and    #
#  conditions stipulated in the agreement/contract under which the program (s)  #
#  have been supplied.                                                          #
#################################################################################

doneMessage()
{
	Message=$1
	[ $# -ne 0 ] || Message='Done'
	ruby -e "puts \"[\x1b[38;32m$Message\x1b[38;0m]\""
}

GetHeaderBlock()
{
	printf "\nSaving Character Set and Header information ................ "	
	cat $1 |sed 's/^$/:block seperator:/g'|sed 's/^/:line seperator:/g'| paste -s -d ''|sed 's/:block seperator:/\n/g' >tmp1
	cat tmp1 | grep "POT-Creation-Date: " | sed  's/:line seperator:/\n/g' > $2
	cat tmp1 | grep -v "POT-Creation-Date: " | sed  's/:line seperator:/\n/g' > $1
	rm tmp1
	doneMessage  
}

SetHeaderBlock()
{
	printf "\nSetting Character Set and Header information ............... "
	cat $2 $1  > tmp1
	mv tmp1 $1 
	rm $2
	doneMessage  
}

ConsistencyCheck()
{
	printf "\nConsistency Check on inpupt file ........................... "
	CheckValue=`cat $1 | grep ':block seperator:\|:line seperator:\|<commented block>\|<double quotes>\|<empty quotes>\|<fuzzy block>\|/<new line>\|<new tab>' | wc -l` 	
 	if [ $CheckValue != "0" ]; then
		doneMessage 'Failed'
		 printf "\n<<Escape Sequences are not handled properly in the script.>>\n\n"
		exit 1
	fi
	doneMessage  
}

EncodeBlocks()
{
	printf "\nEncoding input file......................................... "	
	cat $1	 | sed 's/^#, fuzzy\|^#: , fuzzy/<fuzzy block>/g'    > tmp1 
	cat tmp1 | sed 's/^#~/<commented block>/g' | grep -v "^#"    > tmp2
	cat tmp2 | sed 's/\\"/<double quotes>/g' 					 > tmp3
	cat tmp3 | sed 's/msgid\ ""/msgid\ <empty quotes>/g' | sed 's/msgstr\ ""/msgstr\ <empty quotes>/g' > $1
	rm tmp1 tmp2 tmp3
	doneMessage  
}


ConvertISOToUTF8()
{
    if [ `file $1 |grep UTF-8 |wc -l` == "0" ]; then
       mv $1 tmp
	   iconv -f $2 -t UTF-8 tmp -o $1 > /dev/null 2>&1 
	fi
}

MultiLineBlocksToSingleLineBlocks()
{
	printf "\nRearranging input file...................................... "	
	cat $1   | sed 's/^msgid\ /\nmsgid\ /g' | sed 's/^$/:block seperator:/g' | paste -s -d '' > tmp1
	cat tmp1 | sed 's/<fuzzy block>/:block seperator:<fuzzy block>/g' 			 > tmp2
	cat tmp2 | sed 's/:block seperator:[:block seperator:]*/:block seperator:/g'  > tmp3
	cat tmp3 | sed 's/<fuzzy block>:block seperator:msgid/<fuzzy block>/g'		 > tmp4
	cat tmp4 | sed 's/:block seperator:/\n/g'	| grep -v "^$"					 > $1
	rm tmp1 tmp2 tmp3 tmp4
	doneMessage  
}


IdentifyEmptyBlocks()  
{
	printf "\nIdentifying Empty/Non Empty Blocks.......................... "
	cp $1 tmp1
	cat tmp1 | grep -v "msgstr\ <empty quotes>$\|<fuzzy block>" > $1
	cat tmp1 | grep "msgstr\ <empty quotes>$\|<fuzzy block>" | sed 's/<fuzzy block>/msgid/g' > tmp2
	cat tmp2 | sed 's/msgstr\ \".*\"/msgstr\ <empty quotes>/g' > $2
	rm tmp1 tmp2
	doneMessage  
}

RemoveDumpBlocks()
{	
	printf "\nIdentifying Commented/Incomplete/Inconsistent Blocks........ "
	echo "---------------------Commented Blocks--------------------" > $2
	cat $1   | grep "<commented block>" >> $2
	echo "----------------------Error Blocks-----------------------" >> $2
	cat $1   | grep "msgid\ [^\"]\|msgstr\ [^\"]" | grep -v "<empty quotes>" >> $2
	cat $1   | grep -v  "<commented block>" | sed 's/\\n/<new line>/g' | sed 's/\\t/<new tab>/g'>tmp1
	cat tmp1 | grep "msgid\ \"\|<empty quotes>" | grep "msgstr\ \"\|<empty quotes>" >tmp2
	echo "-----------------Invalid Control Sequences---------------" >> $2
	cat tmp2  | grep "\\\\" >> $2
	cat tmp2  | grep -v "\\\\" | sed 's/<new line>/\\n/g' > $1
	rm tmp1 tmp2
	doneMessage  
}


RemoveDuplicates()
{	
	printf "\nIdentifying Duplicated Blocks............................... "
	cat $1 | sed 's/""\|<empty quotes>//g' > tmp1
  	cat tmp1 | sed 's/msgstr\ /\nmsgstr\ /g' | sed 's/msgid\ /\nmsgid\ /g' | grep "^msgid" | sort > tmp_keys
	cat tmp_keys | uniq > tmp_uniq_keys

	diff tmp_keys tmp_uniq_keys       |grep "<" |sed 's/^< //g' |uniq > tmp_duplicates
	grep -f tmp_duplicates tmp1   | sed 's/<new line>/\\n/g'	> $2
	grep -v -f tmp_duplicates tmp1  | sed 's/<new line>/\\n/g'	> $1

	rm tmp1 tmp_keys tmp_uniq_keys tmp_duplicates  
	doneMessage  
}


SingleLineBlocksToMultiLineBlocks()
{
	cat $1   | sed 's/<empty quotes>/""/g' | sed 's/<commented block>//g' | grep -v "POT-Creation-Date: " > tmp1
	cat tmp1 | sed 's/<new line>/\\n/g' | sed 's/<new tab>/\\t/g' | sed 's/<double quotes>/\\"/g' > tmp2
    cat tmp2 | sed 's/msgstr\ /\nmsgstr\ /g' |sed 's/msgid\ /\nmsgid\ /g'  > $1
	rm tmp1 tmp2
}

if [ $1 != "TESTRUN" ]; then
	if  [ $# -le 1 ]; then
		echo "Usage : poformatter <po_file_name> <current_encoding_format>"
		exit 0
	fi

	PoFile=$1 ;
	EncodingFormat=$2
	if [ ! -f $PoFile ]; then
		echo "File: '$PoFile' does not exists"
		exit 0
	fi

    clear	
	ConsistencyCheck $PoFile
	[ $? -eq 0 ] || exit 0
	ConvertISOToUTF8 $PoFile $EncodingFormat
    GetHeaderBlock $PoFile $PoFile.header	
	EncodeBlocks $PoFile
	MultiLineBlocksToSingleLineBlocks $PoFile
	IdentifyEmptyBlocks $PoFile $PoFile.empty
	RemoveDumpBlocks $PoFile $PoFile.dump
	RemoveDuplicates $PoFile $PoFile.duplicate
	SingleLineBlocksToMultiLineBlocks $PoFile
	SingleLineBlocksToMultiLineBlocks $PoFile.empty
	SingleLineBlocksToMultiLineBlocks $PoFile.dump
	SingleLineBlocksToMultiLineBlocks $PoFile.duplicate
	SetHeaderBlock $PoFile $PoFile.header
	echo ""
fi
