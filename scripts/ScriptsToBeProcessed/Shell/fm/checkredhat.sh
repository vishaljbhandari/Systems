if [ -f /etc/redhat-release ]
then 
	grep "release 9" /etc/redhat-release > /dev/null
else
	echo "1"
fi
echo "$?"
