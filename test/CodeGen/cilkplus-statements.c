// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o - | FileCheck %s

int foo(void);

void test() {
  int a = _Cilk_spawn foo();
  // CHECK: alloca
  // CHECK: call i{{[0-9]*}} @foo()
  // CHECK: store
  a = _Cilk_spawn foo();
  // CHECK: call i{{[0-9]*}} @foo()
  // CHECK: store
  _Cilk_spawn foo();
  // CHECK: call i{{[0-9]*}} @foo()
  _Cilk_sync;
}
