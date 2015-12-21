// RUN: %clang -std=c++11 -fno-exceptions -fcilkplus -emit-llvm -c -S %s -o %t
// RUN: FileCheck --input-file=%t %s
// REQUIRES: cilkplus

void f1(int &v);

void test1() {
  int v = 1;
  _Cilk_spawn f1(v);
  //CHECK: define void @{{.*}}test1
  //CHECK: call void @__cilk_spawn_helper
  //CHECK: cilk.sync.excepting{{[.a-z0-9]*}}:
  //CHECK-NEXT: br label %__cilk_sync.exit
  //CHECK: }
}
