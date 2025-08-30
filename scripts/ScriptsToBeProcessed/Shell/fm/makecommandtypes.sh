rm -f $1/commandtypes.h
touch $1/commandtypes.h

for i in `grep -l Handler *.px`; 
do 
echo "#include \"`basename "$i" .px`_handler.h\"" >>$1/commandtypes.h ; 
done
