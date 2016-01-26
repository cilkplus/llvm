#!/bin/sh

if [ -n "$WIND_BASE" ]; then
echo "\$WIND_BASE is [$WIND_BASE]"
else
echo "\$WIND_BASE is not set"

while true
do
read -p "Input \$WIND_BASE path: " WIND_BASE
if [ -n "$WIND_BASE" ]; then
   break
fi
done

echo "\$WIND_BASE is [$WIND_BASE]"

fi

OLDPATH=$(pwd)

INSTALLDIR=$WIND_BASE/pkgs/os/lang-lib

# echo "target path is $INSTALLDIR"
cd $INSTALLDIR

if [ $(pwd) != "$INSTALLDIR" ]; then
echo "Invalid path"
exit -1
fi

echo "CilkPlus will be installed to $INSTALLDIR\cilk_kernel and $INSTALLDIR\cilk_usr"
echo "Press any key to install..."
read -n 1

rm -rf $INSTALLDIR/cilk_kernel
rm -rf $INSTALLDIR/cilk_usr

mkdir $INSTALLDIR/cilk_kernel
mkdir $INSTALLDIR/cilk_kernel/include
mkdir $INSTALLDIR/cilk_kernel/runtime

cd $INSTALLDIR/cilk_kernel

cp -v -r $OLDPATH/include/* $INSTALLDIR/cilk_kernel/include/
cp -v -r $OLDPATH/runtime/* $INSTALLDIR/cilk_kernel/runtime/
cp -v -r $OLDPATH/mk/vxworks_krnl/* $INSTALLDIR/cilk_kernel/

mkdir $INSTALLDIR/cilk_usr
mkdir $INSTALLDIR/cilk_usr/include
mkdir $INSTALLDIR/cilk_usr/runtime

cp -v -r $OLDPATH/include/* $INSTALLDIR/cilk_usr/include/
cp -v -r $OLDPATH/runtime/* $INSTALLDIR/cilk_usr/runtime/
cp -v -r $OLDPATH/mk/vxworks_usr/* $INSTALLDIR/cilk_usr/

cd $OLDPATH
