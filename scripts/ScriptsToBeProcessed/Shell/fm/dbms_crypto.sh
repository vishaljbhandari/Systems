username=""
password=""
function enableCryptoPackages()
{   echo "set feedback off;
      set verify off; 
      set long 3000;
      set heading off;
      set pagesize 2000;
      spool dbms_crypto.log;
	  @$ORACLE_HOME/rdbms/admin/dbmsobtk.sql
	  @$ORACLE_HOME/rdbms/admin/prvtobtk.plb
	  Grant execute on dbms_crypto to public;
	  Grant execute on dbms_sqlhash to public;
	  Grant execute on dbms_obfuscation_toolkit to public;
	  Grant execute on dbms_obfuscation_toolkit_ffi to public;
	  Grant execute on dbms_crypto_ffi to public;


      spool off;
      exit" | $ORACLE_HOME/bin/sqlplus -s $username/$password as sysdba

}

echo "enter sys user credentials to enable DBMS crypto \n" 
echo "1.YES 2.NO \n"
read choice

if [ $choice -eq 1 ]
then
 	echo "enter the sysdba username"
	read username
	echo "enter the sysdba password"
	read password         
	enableCryptoPackages
else
	exit 0
fi

