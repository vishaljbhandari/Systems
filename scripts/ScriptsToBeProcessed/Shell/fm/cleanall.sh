rm -f `find . -name commandgets.h -o -name commandputs.h -o -name commandtypes.h`

for i in `find . -name \*.dh`
do
rm -f `dirname $i`/`basename $i .dh`db.h
rm -f `dirname $i`/`basename $i .dh`db.cc
rm -f `dirname $i`/`basename $i .dh`db.o
done

for i in `find . -name \*.dame`
do
rm -f `dirname $i`/`basename $i .dame`db.h
rm -f `dirname $i`/`basename $i .dame`db.cc
rm -f `dirname $i`/`basename $i .dame`db.o
done

for i in `find . -name \*.x`
do
rm -f `dirname $i`/`basename $i .x`.cc
rm -f `dirname $i`/`basename $i .x`.o
rm -f `dirname $i`/`basename $i .x`.h
rm -f `dirname $i`/`basename $i .x`_handler.cc
rm -f `dirname $i`/`basename $i .x`_handler.o
rm -f `dirname $i`/`basename $i .x`_handler.h
rm -f `dirname $i`/`basename $i .x`_stub.cc
rm -f `dirname $i`/`basename $i .x`_stub.o
rm -f `dirname $i`/`basename $i .x`_stub.h
rm -f `dirname $i`/`basename $i .x`.gx
rm -f `dirname $i`/`basename $i .x`.px
done
