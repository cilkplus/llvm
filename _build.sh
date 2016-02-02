CLANG_HOME=`pwd`/new-git-cilk-Feb-2-3
BUILD_HOME=$CLANG_HOME/build

git clone -b cilkplus https://github.com/cilkplus/llvm $CLANG_HOME
git clone -b cilkplus https://github.com/cilkplus/clang $CLANG_HOME/tools/clang
git clone -b cilkplus https://github.com/cilkplus/compiler-rt $CLANG_HOME/projects/compiler-rt

rm -rf $BUILD_HOME
mkdir $BUILD_HOME
cd $BUILD_HOME

cmake -G "Unix Makefiles" -DINTEL_SPECIFIC_CILKPLUS=1 -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=1 .. 2>&1 | tee config.log
make -j 8 2>&1 | tee build.log
export PATH=$BUILD_HOME/bin:$PATH

mkdir cilkrt
cd cilkrt
cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=$BUILD_HOME $CLANG_HOME/projects/compiler-rt/lib/cilk 2>&1 | tee config.log
make 2>&1 | tee build.log
make install 2>&1 | tee install.log
