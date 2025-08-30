#! /bin/bash
. poFormatter.sh "TESTRUN" 
. shUnit

compareFiles()
{ 
  IsSame=""
  [ `diff $1 $2 | wc -l` == "0" ] ||  IsSame="1"
  shuAssert "$3" $IsSame
  	  
}

getFiles()
{
	dataFile="./Tests/testPoData.dat"
	grep "$1"  $dataFile | sed "s/$1:  //g"> input.po 
	grep "$2"  $dataFile | sed "s/$2:  //g"> result_1.po
	if  [ $# -eq 3 ]; then
		grep "$3"  $dataFile | sed "s/$3:  //g"> result_2.po
	fi
      
}

function TestEncodeBlocks()
{
  	  getFiles "FILE_01" "FILE_02"

      EncodeBlocks input.po > /dev/null 2>&1
	  compareFiles input.po result_1.po "EncodeBlocks"
	  
	  rm input.po result_1.po  > /dev/null 2>&1
}

function TestMultiLineBlocksToSingleLineBlocks()
{
  	  getFiles "FILE_03" "FILE_04" 

      MultiLineBlocksToSingleLineBlocks input.po  > /dev/null 2>&1
	  compareFiles input.po result_1.po "MultiLineBlocksToSingleLineBlocks"
	  
	  rm input.po result_1.po
}

function TestIdentifyEmptyBlocks()
{
  	  getFiles "FILE_05" "FILE_06" "FILE_07" 

      IdentifyEmptyBlocks input.po empty.po  > /dev/null 2>&1
	  compareFiles input.po result_1.po  "IdentifyEmptyBlocks - Non Empty Check"
	  compareFiles empty.po result_2.po  "IdentifyEmptyBlocks - Empty Check"
	  
	  rm input.po empty.po result_1.po result_2.po  > /dev/null 2>&1
	
}

function TestRemoveDumpBlocks()
{
  	  getFiles "FILE_08" "FILE_09" "FILE_10" 
		  
      RemoveDumpBlocks input.po dump.po  > /dev/null 2>&1
	  compareFiles input.po result_1.po "RemoveDumpBlocks - Non Dump Check"
	  compareFiles dump.po  result_2.po "RemoveDumpBlocks - Dump Check"
	  
	  rm input.po result_1.po result_2.po dump.po  > /dev/null 2>&1
}

function TestRemoveDuplicates()
{
	  getFiles "FILE_11" "FILE_12" "FILE_13" 
		  
      RemoveDuplicates input.po duplicate.po  > /dev/null 2>&1
	  compareFiles input.po result_1.po "TestRemoveDuplicates - Non Duplicate Check"
	  compareFiles duplicate.po  result_2.po "TestRemoveDuplicates - Duplicate Check"
	  
	  rm input.po result_1.po result_2.po duplicate.po  > /dev/null 2>&1
}


function TestSingleLineBlocksToMultiLineBlocks
{
  	  getFiles "FILE_14" "FILE_15" 
		  
      SingleLineBlocksToMultiLineBlocks input.po  > /dev/null 2>&1
	  compareFiles input.po result_1.po "TestSingleLineBlocksToMultiLineBlocks"
	  
	  rm input.po result_1.po  > /dev/null 2>&1
}



function TestGetHeaderBlock()
{
  	  getFiles "FILE_16" "FILE_17" "FILE_18"

      GetHeaderBlock input.po header.po  > /dev/null 2>&1
	  compareFiles header.po result_1.po "TestGetHeaderBlock - Header Check"
	  compareFiles input.po  result_2.po "TestGetHeaderBlock - Body Check"
		  
	  rm input.po header.po result_1.po result_2.po > /dev/null 2>&1
}


function InitFunction
{
	shuRegTest TestEncodeBlocks
	shuRegTest TestMultiLineBlocksToSingleLineBlocks
	shuRegTest TestIdentifyEmptyBlocks
	shuRegTest TestRemoveDumpBlocks
	shuRegTest TestRemoveDuplicates
	shuRegTest TestSingleLineBlocksToMultiLineBlocks
	shuRegTest TestGetHeaderBlock
}

shuStart InitFunction

