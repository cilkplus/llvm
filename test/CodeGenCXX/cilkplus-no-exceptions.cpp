// RUN: %clang -std=c++11 -fno-exceptions -fcilkplus -emit-llvm -c -S %s -o %t
// RUN: FileCheck --input-file=%t %s

void f1(int &v);

void test1() {
  int v = 1;
  _Cilk_spawn f1(v);
  //CHECK: define void @_Z5test1v
  //CHECK-NOT: invoke
  //CHECK: }
}