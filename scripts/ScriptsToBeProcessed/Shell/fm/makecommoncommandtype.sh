rm -f $1/commandcommontypes.h
touch $1/commandcommontypes.h

for i in `grep -l protocol $1/*.x`
do
echo "#include \"`basename "$i" .x`_handler.h\"" >> $1/commandcommontypes.h
done
