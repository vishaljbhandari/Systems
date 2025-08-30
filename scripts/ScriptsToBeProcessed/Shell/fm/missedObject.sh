#! /bin/bash

. rangerenv.sh


User=$1
Password=$2
User2=$3
Password2=$4
RunPath=`dirname $0`

if [ $# -lt 4 ]
then
	echo "Usage  $0 <DB_USER1> <DB_PASSWORD1> <DB_USER2> <DB_PASSWORD2>  >"
    exit
fi


cd $RunPath


# get all views
>viewList.sql
>userView1.log
>userView2.log
echo "set pages 1000;" >> viewList.sql
fetchSql="select tname from tab WHERE TABTYPE ='VIEW' order by tname ;"

echo $fetchSql >> viewList.sql

echo "exit" >> viewList.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @viewList.sql >> userView1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @viewList.sql >> userView2.log 2>&1

# get all table
>tableList.sql
>userTable1.log
>userTable2.log
echo "set pages 1000;" >> tableList.sql
fetchSql="select tname from tab WHERE TABTYPE ='TABLE' order by tname ;"

echo $fetchSql >> tableList.sql

echo "exit" >> tableList.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @tableList.sql >> userTable1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @tableList.sql >> userTable2.log 2>&1

# get all cluster
>clusterList.sql
>userCluster1.log
>userCluster2.log
echo "set pages 1000;" >> clusterList.sql
fetchSql="select tname from tab WHERE TABTYPE ='CLUSTER' order by tname ;"

echo $fetchSql >> clusterList.sql

echo "exit" >> clusterList.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @clusterList.sql >> userCluster1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @clusterList.sql >> userCluster2.log 2>&1


#get all procedures
>procedurelist.sql
>userProcedure1.log
>userProcedure2.log
echo "set pages 1000;" >> procedurelist.sql
fetchSql="select OBJECT_NAME from user_objects where OBJECT_TYPE='PROCEDURE'order by OBJECT_NAME ;"
echo $fetchSql >> procedurelist.sql
echo "exit" >> procedurelist.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @procedurelist.sql >> userProcedure1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @procedurelist.sql >> userProcedure2.log 2>&1


#get all function
>Functionlist.sql
>userFunction1.log
>userFunction2.log
echo "set pages 1000;" >> Functionlist.sql
fetchSql="select OBJECT_NAME from user_objects where OBJECT_TYPE='FUNCTION'order by OBJECT_NAME ;"
echo $fetchSql >> Functionlist.sql
echo "exit" >> Functionlist.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @Functionlist.sql >> userFunction1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @Functionlist.sql >> userFunction2.log 2>&1



#get all type
>Typelist.sql
>userType1.log
>userType2.log
echo "set pages 1000;" >> Typelist.sql
fetchSql="select OBJECT_NAME from user_objects where OBJECT_TYPE='TYPE'order by OBJECT_NAME ;"
echo $fetchSql >> Typelist.sql
echo "exit" >> Typelist.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @Typelist.sql >> userType1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @Typelist.sql >> userType2.log 2>&1


#get all trigger
>Triggerlist.sql
>userTrigger1.log
>userTrigger2.log
echo "set pages 1000;" >> Triggerlist.sql
fetchSql="select OBJECT_NAME from user_objects where OBJECT_TYPE='TRIGGER'order by OBJECT_NAME ;"
echo $fetchSql >> Triggerlist.sql
echo "exit" >> Triggerlist.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @Triggerlist.sql >> userTrigger1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @Triggerlist.sql >> userTrigger2.log 2>&1



#get all Package
>Packagelist.sql
>userPackage1.log
>userPackage2.log
echo "set pages 1000;" >> Packagelist.sql
fetchSql="select OBJECT_NAME from user_objects where OBJECT_TYPE='PACKAGE'order by OBJECT_NAME ;"
echo $fetchSql >> Packagelist.sql
echo "exit" >> Packagelist.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @Packagelist.sql >> userPackage1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @Packagelist.sql >> userPackage2.log 2>&1

#get all Sequence
>Sequencelist.sql
>userSequence1.log
>userSequence2.log
echo "set pages 1000;" >> Sequencelist.sql
fetchSql="select OBJECT_NAME from user_objects where OBJECT_TYPE='SEQUENCE'order by OBJECT_NAME ;"
echo $fetchSql >> Sequencelist.sql
echo "exit" >> Sequencelist.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @Sequencelist.sql >> userSequence1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @Sequencelist.sql >> userSequence2.log 2>&1



#get all index
>Indexlist.sql
>userIndex1.log
>userIndex2.log
echo "set pages 1000;" >> Indexlist.sql
fetchSql="select OBJECT_NAME from user_objects where OBJECT_TYPE='INDEX'order by OBJECT_NAME ;"
echo $fetchSql >> Indexlist.sql
echo "exit" >> Indexlist.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @Indexlist.sql >> userIndex1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @Indexlist.sql >> userIndex2.log 2>&1


#get all Tablespaces
>Tablespaceslist.sql
>userTablespaces1.log
>userTablespaces2.log
echo "set pages 1000;" >> Indexlist.sql
fetchSql="select TABLESPACE_NAME from user_tablespaces;"
echo $fetchSql >> Tablespaceslist.sql
echo "exit" >> Tablespaceslist.sql

$SQL_COMMAND -s $User/$Password$DB_CONNECTION_STRING @Tablespaceslist.sql >> userTablespaces1.log 2>&1
$SQL_COMMAND -s $User2/$Password2$DB_CONNECTION_STRING @Tablespaceslist.sql >> userTablespaces2.log 2>&1



DB_STATUS=$?

	sed -i '/^--/ d' userCluster1.log
	sed -i '/^TNAME/ d' userCluster1.log
	sed -i '/^[0-9]/ d' userCluster1.log
    sed -i '/^\s*$/d' userCluster1.log

	sed -i '/^--/ d' userCluster2.log
	sed -i '/^TNAME/ d' userCluster2.log
	sed -i '/^[0-9]/ d' userCluster2.log
	sed -i '/^\s*$/d' userCluster2.log


    diff userCluster1.log userCluster2.log > missedCluster.log


	sed -i '/^--/ d' userTable1.log
	sed -i '/^TNAME/ d' userTable1.log
	sed -i '/^[0-9]/ d' userTable1.log
	sed -i '/^\s*$/d' userTable1.log

	sed -i '/^--/ d' userTable2.log
	sed -i '/^TNAME/ d' userTable2.log
	sed -i '/^[0-9]/ d' userTable2.log
	sed -i '/^\s*$/d' userTable2.log
    
	diff userTable1.log userTable2.log > missedTable.log
	
	sed -i '/^--/ d' userView1.log
	sed -i '/^TNAME/ d' userView1.log
	sed -i '/^[0-9]/ d' userView1.log
	sed -i '/^\s*$/d' userView1.log

	sed -i '/^--/ d' userView2.log
	sed -i '/^TNAME/ d' userView2.log
	sed -i '/^[0-9]/ d' userView2.log
	sed -i '/^\s*$/d' userView2.log

	diff userView1.log userView2.log > missedView.log

	sed -i '/^--/ d' userTrigger1.log
	sed -i '/^TNAME/ d' userTrigger1.log
	sed -i '/^[0-9]/ d' userTrigger1.log
    	sed -i '/^\s*$/d' userTrigger1.log

	sed -i '/^--/ d' userTrigger2.log
	sed -i '/^TNAME/ d' userTrigger2.log
	sed -i '/^[0-9]/ d' userTrigger2.log
	sed -i '/^\s*$/d' userTrigger2.log


    diff userTrigger1.log userTrigger2.log > missedTrigger.log

	sed -i '/^--/ d' userFunction1.log
	sed -i '/^TNAME/ d' userFunction1.log
	sed -i '/^[0-9]/ d' userFunction1.log
    	sed -i '/^\s*$/d' userFunction1.log

	sed -i '/^--/ d' userFunction2.log
	sed -i '/^TNAME/ d' userFunction2.log
	sed -i '/^[0-9]/ d' userFunction2.log
	sed -i '/^\s*$/d' userFunction2.log


    diff userFunction1.log  userFunction2.log  > missedFunction.log


	sed -i '/^--/ d' userPackage1.log
	sed -i '/^TNAME/ d' userPackage1.log
	sed -i '/^[0-9]/ d' userPackage1.log
   	 sed -i '/^\s*$/d' userPackage1.log

	sed -i '/^--/ d' userPackage2.log
	sed -i '/^TNAME/ d' userPackage2.log
	sed -i '/^[0-9]/ d' userPackage2.log
	sed -i '/^\s*$/d' userPackage2.log


    diff userPackage1.log  userPackage2.log  > missedPackage.log


	sed -i '/^--/ d' userType1.log
	sed -i '/^TNAME/ d' userType1.log
	sed -i '/^[0-9]/ d' userType1.log
    	sed -i '/^\s*$/d' userType1.log

	sed -i '/^--/ d' userType2.log
	sed -i '/^TNAME/ d' userType2.log
	sed -i '/^[0-9]/ d' userType2.log
	sed -i '/^\s*$/d' userType2.log


    diff userType1.log userType2.log > missedType.log

	sed -i '/^--/ d' userProcedure1.log
	sed -i '/^TNAME/ d' userProcedure1.log
	sed -i '/^[0-9]/ d' userProcedure1.log
   	sed -i '/^\s*$/d' userProcedure1.log

	sed -i '/^--/ d' userProcedure2.log
	sed -i '/^TNAME/ d' userProcedure2.log
	sed -i '/^[0-9]/ d' userProcedure2.log
	sed -i '/^\s*$/d' userProcedure2.log


    diff userProcedure1.log userProcedure2.log > missedProcedure.log

	sed -i '/^--/ d' userSequence1.log
	sed -i '/^TNAME/ d' userSequence1.log
	sed -i '/^[0-9]/ d' userSequence1.log
   	sed -i '/^\s*$/d' userSequence1.log

	sed -i '/^--/ d' userSequence2.log
	sed -i '/^TNAME/ d' userSequence2.log
	sed -i '/^[0-9]/ d' userSequence2.log
	sed -i '/^\s*$/d' userSequence2.log


    diff userSequence1.log userSequence2.log > missedSequence.log



	sed -i '/^--/ d' userIndex1.log
	sed -i '/^TNAME/ d' userIndex1.log
	sed -i '/^[0-9]/ d' userIndex1.log
   	sed -i '/^\s*$/d' userIndex1.log

	sed -i '/^--/ d' userIndex2.log
	sed -i '/^TNAME/ d' userIndex2.log
	sed -i '/^[0-9]/ d' userIndex2.log
	sed -i '/^\s*$/d' userIndex2.log


    diff userIndex1.log  userIndex2.log > missedIndex.log



	sed -i '/^--/ d' userTablespaces1.log
	sed -i '/^TNAME/ d' userTablespaces1.log
	sed -i '/^[0-9]/ d' userTablespaces1.log
   	sed -i '/^\s*$/d' userTablespaces1.log

	sed -i '/^--/ d' userTablespaces2.log
	sed -i '/^TNAME/ d' userTablespaces2.log
	sed -i '/^[0-9]/ d' userTablespaces2.log
	sed -i '/^\s*$/d' userTablespaces2.log


    diff userTablespaces1.log  userTablespaces2.log > missedtablespaces.log

echo
echo "GenerateList Completed."

