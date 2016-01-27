#!/bin/bash

###### This file keeps the script to build Linux version of CilkPlus Clang based compiler

###### By default we assume you're using 'llvm' folder created in the current directory as a base folder of your source installation
###### If you use some specific location then pass it as argument to this script

if [ "$1" = "" ]
then
  LLVM_HOME=`pwd`/llvm
else
  LLVM_HOME=$1
fi

echo Building $LLVM_HOME...

###### Uncommet the following 4 lines if you'd like to build the project from scratch
###### If you already have the sources leave the following lines w/o changes
rm -rf $LLVM_HOME
git clone -b cilkplus https://github.com/cilkplus/llvm $LLVM_HOME
git clone -b cilkplus https://github.com/cilkplus/clang $LLVM_HOME/tools/clang
git clone -b cilkplus https://github.com/cilkplus/compiler-rt $LLVM_HOME/projects/compiler-rt

BUILD_HOME=$LLVM_HOME/build
mkdir -p $BUILD_HOME
cd $BUILD_HOME

###### If you need to tune your environment - do it
#export PATH=/usr/local/bin:/usr/bin:$PATH
#export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
../configure --with-gcc-toolchain=/usr/local

###### By default you should simply lanch
#../configure

###### Now you're able to build the compiler
make -j8 >build.log

###### The following lines will prepare your system to build CilkPlus runtime
###### If you don't want to do it - simply finish the script here
###### (remove or comment all the rest lines of the script)
###### We're suggesting to use your own Clang version to build the runtime
###### that's why we're exporting the following 3 variables
export PATH=$BUILD_HOME/Debug+Asserts/bin:$PATH
export CC=clang
export CXX=clang++

###### That's the standard place in the llvm structure to deal with cilk runtime library
CILK_RT_HOME=$LLVM_HOME/projects/compiler-rt/lib/cilk

###### We're getting the latest sources from Cilk runtime site
###### that's why we suggest to remove all the stuff from our own code snapshot in the source tree
rm -rf $CILK_RT_HOME
git clone https://bitbucket.org/intelcilkruntime/intel-cilk-runtime.git $CILK_RT_HOME
cd $CILK_RT_HOME

###### You could find the instruction on how to build cilk runtime in $CILK_RT_HOME/README
libtoolize
aclocal
automake --add-missing
autoconf

###### By default we'll install libraries and include files in $BUILD_HOME
###### If you don't like it - remove --prefix or use your prefered location
./configure --prefix=$BUILD_HOME

make
make install

###### That's it!
###### But don't forget to extend LD_LIBRARY_PATH and INCLUDE search path with selected --prefix/lib|include
###### (if you used the one). For example:
###### export LD_LIBRARY_PATH=$BUILD_HOME/lib:$LD_LIBRARY_PATH
###### export CFLAGS=$BUILD_HOME/include:$CFLAGS
###### export CXXFLAGS=$BUILD_HOME/include:$CXXFLAGS
