// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o - | FileCheck %s

void test_grainsize() {
  #pragma cilk grainsize = 2
  _Cilk_for(int i = 0; i < 10; ++i);
  // CHECK: call void @__cilkrts_cilk_for_32({{.*}}, i32 %{{[A-Za-z0-9]+}}, i32 2

  long gs = 4;
  #pragma cilk grainsize = gs
  _Cilk_for(int i = 0; i < 10; ++i);
  // CHECK: store i64 4, i64* [[GS:%[A-Za-z0-9]+]]
  // CHECK-NEXT: [[TMP:%[A-Za-z0-9]+]] = load i64* [[GS]],
  // CHECK-NEXT: [[CONV:%[A-Za-z0-9]+]] = trunc i64 [[TMP]] to i32
  // CHECK: call void @__cilkrts_cilk_for_32({{.*}}, i32 %{{[A-Za-z0-9]+}}, i32 [[CONV]]

  // If we don't specify grainsize, it should be 0
  _Cilk_for(int i = 0; i < 10; ++i);
  // CHECK: call void @__cilkrts_cilk_for_32({{.*}}, i32 %{{[A-Za-z0-9]+}}, i32 0
}
