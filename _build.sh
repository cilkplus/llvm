CILK_HOME=git-cilk
git clone -b cilkplus https://github.com/cilkplus/llvm $CILK_HOME
git clone -b cilkplus https://github.com/cilkplus/clang $CILK_HOME/tools/clang
git clone -b cilkplus https://github.com/cilkplus/compiler-rt $CILK_HOME/projects/compiler-rt

cd $CILK_HOME
mkdir build
cd build
cmake -G "Unix Makefiles" -DINTEL_SPECIFIC_CILKPLUS=1 -DLLVM_ENABLE_ASSERTIONS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=1 .. 2>&1 | tee config.log
make -j 8 2>&1 | tee build.log
make -j 8 check-clang 2>&1 | tee check-clang.log
