// RUN: %clang_cc1 -fcilkplus -triple x86_64-windows-msvc -emit-llvm %s -o - | FileCheck --check-prefix=X64 %s
// REQUIRES: cilkplus

int foo ();

int test_spawn() {
  // X64-LABEL: define{{.*}}test_spawn
  // CHECK-NOT: llvm.eh.sjlj
  int i = _Cilk_spawn foo();
  // X64: %[[addr:.*]] = call{{.*}}llvm.frameaddress
  // X64: %[[addr:.*]] = call{{.*}}llvm.stacksave
  // X64: %[[call:.*]] = call{{.*}}setjmp
  // X64: call{{.*}}__cilk_spawn_helper
  // X64: %[[addr:.*]] = call{{.*}}llvm.frameaddress
  // X64: %[[addr:.*]] = call{{.*}}llvm.stacksave
  // X64: %[[call:.*]] = call{{.*}}setjmp
  // X64: call{{.*}}__cilkrts_sync
  return i;
}

