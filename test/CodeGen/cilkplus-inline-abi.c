// RUN: %clang_cc1 -fcilkplus -O2 -emit-llvm %s -o - | FileCheck %s

void foo(void);

void test() {
  _Cilk_spawn foo();
  // CHECK-NOT: call void @__cilkrts_enter_frame
  // CHECK-NOT: call void @__cilkrts_pop_frame
  // CHECK-NOT: call void @__cilkrts_detach
  // CHECK-NOT: call void @__cilk_sync
}
