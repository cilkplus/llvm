// RUN: %clang_cc1 -std=c++11 -fcilkplus -fcxx-exceptions -fexceptions  -disable-llvm-optzns -emit-llvm %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -fcxx-exceptions -fexceptions  -disable-llvm-optzns -emit-llvm %s -o - | FileCheck -check-prefix=CHECK1 %s
// XFAIL: *
// REQUIRES: cilkplus

void test_cilk_for_body() {
  // Confirm this compiles; do not check the generated code.
  _Cilk_for (int i = 0; i < 10; i++) {
    try { throw 2013; } catch (...) { }
  }
}

void foo();

void test_cilk_for_try_cilk_spawn() {
  _Cilk_for (int i = 0; i < 10; i++) {
    try {
      _Cilk_spawn foo();
    } catch (...) { }
  }
  // CHECK: define void @{{.*}}test_cilk_for_try_cilk_spawn
  // CHECK-NOT: call void @__cilk_parent_prologue
  // CHECK: call void @__cilkrts_cilk_for_32({{.*}} [[helper1:@__cilk_for_helper[0-9]*]]

  // CHECK: define internal void [[helper1]]
  // CHECK: call void @__cilk_parent_prologue
  //
  // First implicit sync before entering the try-block
  // CHECK: invoke void @__cilk_sync
  //
  // CHECK: invoke void @__cilk_spawn_helper
  //
  // Second implicit sync before existing the try-block, normal path
  // CHECK: invoke void @__cilk_sync
  //
  // Thrid implicit sync before existing the try-block, exception path
  // CHECK: @__gxx_personality_v
  // CHECK: invoke void @__cilk_sync
  //
  // No implicit sync until the end of the cilk for helper function, normal or exception path
  // CHECK: call void @__cilk_parent_epilogue
  // CHECK: call void @__cilk_parent_epilogue
  // CHECK-NOT:  call void @__cilk_sync
  //
  // CHECK: resume { i8*, i32 }
}

void test_cilk_for_catch_cilk_spawn() {
  _Cilk_for (int i = 0; i < 10; i++) {
    try {
      foo();
    } catch (...) {
      _Cilk_spawn foo();
    }
  }
  // CHECK: define void @{{.*}}test_cilk_for_catch_cilk_spawn
  // CHECK-NOT: call void @__cilk_parent_prologue
  // CHECK: call void @__cilkrts_cilk_for_32({{.*}} [[helper2:@__cilk_for_helper[0-9]*]]
  //
  // CHECK: define internal void [[helper2]](
  // CHECK: call void @__cilk_parent_prologue
  //
  // CHECK: invoke void @__cxa_end_catch()
  //
  // An implicit sync is needed for the cilk for helper function, normal or exception path.
  //
  // CHECK: invoke void @__cilk_sync
  // CHECK: call void @__cilk_parent_epilogue
  // CHECK-NEXT: ret void
  //
  // CHECK: invoke void @__cilk_sync
  // CHECK: call void @__cilk_parent_epilogue
}

namespace cilk_for_exceptions {

void anchor(int) throw();

void throw_odd(int n) {
  if (n % 2) throw n;
}

void test1() {
  anchor(10);
  _Cilk_for (int i = 0; i < 10; ++i) {
    throw_odd(i);
  }
  anchor(11);
  // CHECK1: call void @{{.*}}cilk_for_exceptions{{.*}}anchor
  // CHECK1: call void @__cilkrts_cilk_for_32
  // CHECK1-NOT: __cilk_sync
  // CHECK1: call void @{{.*}}cilk_for_exceptions{{.*}}anchor
}

void test2() {
  anchor(12);
  _Cilk_for (int i = 0; i < 10; ++i) {
    _Cilk_spawn throw_odd(i);
  }
  anchor(13);
  // CHECK1: call void @{{.*}}cilk_for_exceptions{{.*}}anchor
  // CHECK1: call void @__cilkrts_cilk_for_32({{.*}} @[[Helper:__cilk_for_helper[0-9]*]]
  // CHECK1: call void @{{.*}}cilk_for_exceptions{{.*}}anchor
  // CHECK1-NEXT: ret void
  //
  // CHECK1: define internal void @[[Helper]]
  // CHECK1: call void @__cilk_parent_prologue
  //
  // CHECK1: invoke void @__cilk_sync
  // CHECK1: call void @__cilk_parent_epilogue
  // CHECK1-NEXT: ret void
}

} // namespace

