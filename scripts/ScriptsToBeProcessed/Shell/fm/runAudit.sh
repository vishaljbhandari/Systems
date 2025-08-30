#!/bin/sh

GetMissingColumns()
{
	for var in `cat common_tables.lst`
	do
		# find the number of columns for the table in each schema
		cols1=`cat dbschema_$1.lst | grep "[0-9]$" | awk '{print $1}' | grep -w $var | grep -v IX | wc -l`
		cols2=`cat dbschema_$2.lst | grep "[0-9]$" | awk '{print $1}' | grep -w $var | grep -v IX | wc -l`
		
		# Check if number of columns matches or not
		if [ $cols1 -ne $cols2 ] ; then
			echo "COLUMNS MISMATCH for " $var >> cols.txt
		fi

		rows1=`cat tables_$1.lst | grep -w $var | awk '{ print $2 }'`
		rows2=`cat tables_$2.lst | grep -w $var | awk '{ print $2 }'`

		if [ "$rows1" -ne "$rows2" ] ; then
            echo "ROWS MISMATCH for " $var >> rows.txt
        fi
	done
}

GetMissingEntries()
{
	 rm -f common_tables.lst

	 echo $3 " Missing in " $2 " schema";

     for var in `cat $3_$1.lst | grep -v "rows selected" | awk '{print $1}'`
     do
        cat $3_$2.lst | grep -w $var >/dev/null
        if [ $? -ne 0  ]; then
			echo $var
		else
			echo $var >> common_tables.lst
        fi
     done

	 echo "---------------------------------------------------------------------------------------------"	

	 echo $3 " Missing in " $1" schema";

     for var in `cat $3_$2.lst | grep -v "rows selected" | awk '{print $1}'` 
     do
        cat $3_$1.lst | grep -w $var >/dev/null
        if [ $? -ne 0  ]; then
            echo $var
        fi
     done
}

GetMissingObjects()
{
     for var in `cat common_tables.lst`
     do
			cat obj_$3_$1.lst | grep -w $var | awk '{ print$NF }'  | sort > du1.txt
			cat obj_$3_$2.lst | grep -w $var | awk '{ print$NF }'  | sort > du2.txt

			diff du1.txt du2.txt > tmp.txt

			ret_val=$?

			if [ $ret_val -eq 1 ]; then
				diff du1.txt du2.txt | grep "<" | awk '{ print $NF }'  > tmp.txt
				echo "The " $4 " missing for " $var " in " $2 " are"
				cat tmp.txt
				echo "-----------------------------------------------------------------------------------------------------------"

				diff du1.txt du2.txt | grep ">" | awk '{ print $NF }' > tmp.txt
                echo "The " $4 " missing for " $var " in " $1 " are"
                cat tmp.txt
				echo "##########################################################################################################"
			fi
     done
}

GetCount()
{
	cnt1=`cat dbschema_$1.lst | grep -w "$3" | head -n 1 | awk '{ print $NF }'`;
	cnt2=`cat dbschema_$2.lst | grep -w "$3" | head -n 1 | awk '{ print $NF }'`;

	echo $3"s in " $1 " schema : " $cnt1
	echo $3"s in " $2 " schema : " $cnt2

	diff=`expr $cnt1 - $cnt2`;
}

PrintDashedLine()
{
	echo "======================================================================================================================";
}

if [ $# -ne 2 ] ;then
	echo "InSufficient Parematers. Make sure that you have run get_schema.sql for both the databases which you wish to
	compare"
	echo "Syntax is ./runAudit ARG1 ARG2";
	echo "where"
	echo -e "\t\t ARG1 = Argument passed to get_schema.sql in source db"
	echo -e "\t\t ARG2 = Argument passed to get_schema.sql in target db"

	exit 1;
fi

touch cols.txt rows.txt

PrintDashedLine

GetCount $1 $2 "TABLE"

GetMissingEntries $1 $2 "tables"

PrintDashedLine

GetMissingColumns $1 $2

PrintDashedLine

GetCount $1 $2 "INDEX"

PrintDashedLine

GetMissingEntries $1 $2 "indexes"

PrintDashedLine

GetCount $1 $2 "SEQUENCE"

GetMissingEntries $1 $2 "sequences"

PrintDashedLine
GetCount $1 $2 "VIEW"
PrintDashedLine

GetMissingEntries $1 $2 "views"

PrintDashedLine
cat cols.txt 
PrintDashedLine
cat rows.txt
PrintDashedLine

cnt1=`cat tablespaces_$1.lst | grep "rows selected" | awk '{print $1}'`
echo TABLESPACE"s in $1 schema : " $cnt1

cnt2=`cat tablespaces_$2.lst | grep "rows selected" | awk '{print $1}'`
echo TABLESPACE"s in $2 schema : " $cnt2

diff=`expr $cnt1 - $cnt2`;
PrintDashedLine

GetMissingEntries $1 $2 "tablespaces"
PrintDashedLine
echo "Tables missing for Table spaces common to both the schemas"
GetMissingObjects $1 $2 tables tables
echo "Partitions missing for Table spaces common to both the schemas"
GetMissingObjects $1 $2 ts_partn_name partitions
echo "Sub Partitions missing for Table Spaces common to both the schemas"
GetMissingObjects $1 $2 ts_sub_partn_name subpartitions

PrintDashedLine
GetCount $1 $2 "TABLE PARTITION"
PrintDashedLine

GetMissingEntries $1 $2 "partitions"		
PrintDashedLine
echo "Partitions missing for objects common to both the schemas"
GetMissingObjects $1 $2 partitions partitions

PrintDashedLine
GetCount $1 $2 "TABLE SUBPARTITION"
PrintDashedLine

GetMissingEntries $1 $2 "subpartitions"
PrintDashedLine
echo "SubPartitions missing for objects common to both the schemas"
GetMissingObjects $1 $2 subpartitions subpartitions

PrintDashedLine
GetCount $1 $2 "PROCEDURE"
PrintDashedLine

GetMissingEntries $1 $2 "procedures"
PrintDashedLine
echo "Procedures missing for objects common to both the schemas"
PrintDashedLine
GetMissingObjects $1 $2 procedures procedures

PrintDashedLine
GetCount $1 $2 "PACKAGE"
PrintDashedLine
GetMissingEntries $1 $2 "packages"
PrintDashedLine

GetCount $1 $2 "FUNCTION"
PrintDashedLine
GetMissingEntries $1 $2 "functions"
PrintDashedLine

echo "Privileges granted to User of source database"
cat user_privs_$2.lst | grep -v "rows selected"

PrintDashedLine

echo "Privileges granted to User of target database"
cat user_privs_$3.lst | grep -v "rows selected"

PrintDashedLine

echo "Roles granted to User of source database"
cat user_roles_$2.lst | grep -v "rows selected"

PrintDashedLine

echo "Roles granted to User of target database"
cat user_roles_$3.lst | grep -v "rows selected"

rm -rf cols.txt rows.txt common_tables.lst du1.txt du2.txt tmp.txt

