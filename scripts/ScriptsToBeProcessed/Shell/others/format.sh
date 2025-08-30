umount p10  # just in case
if
mkfs -t $1 /dev/sda10
then
mount /dev/sda10 p10
else
echo failed to format
fi


