// RUN: %clang_cc1 -fcilkplus -O2 -emit-llvm %s -o - | FileCheck %s

void foo(void);

void test() {
  _Cilk_spawn foo();
  // CHECK-NOT: call void @__cilkrts_enter_frame
  // CHECK-NOT: call void @__cilkrts_pop_frame
  // CHECK-NOT: call void @__cilkrts_detach
  // CHECK-NOT: call void @__cilk_sync
  // CHECK: ret void
}

void test_notify_intrinsics() {
  __notify_intrinsic(0, 0);
  __notify_zc_intrinsic(0, 0);
  // FIXME: above intrinsics not implemented (LE-2437)
  //
  // CHECK: define void @test_notify_intrinsics
  // CHECK-NOT: call
  // CHECK-NOT: br
  // CHECK: ret void
}
