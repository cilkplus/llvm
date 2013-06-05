// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck %s

void test() {
  #pragma simd
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !0
// CHECK: llvm.mem.parallel_loop_access !0
// CHECK: llvm.loop !0
}

void test2(int *A, int N) {
  int *IE = A + N;
  #pragma simd
  for (int *I = A; I != IE; ++I)
    *I *= 2;
// CHECK: llvm.mem.parallel_loop_access !1
// CHECK: llvm.mem.parallel_loop_access !1
// CHECK: llvm.loop !1
}

constexpr int width(int i) { return 1 << i; }

void test_vectorlength() {
  #pragma simd vectorlength(4)
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !2
// CHECK: llvm.mem.parallel_loop_access !2
// CHECK: llvm.loop !2

  const int W = 1 << 3;
  #pragma simd vectorlength(W)
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !4
// CHECK: llvm.mem.parallel_loop_access !4
// CHECK: llvm.loop !4

  #pragma simd vectorlength(width(4))
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !6
// CHECK: llvm.mem.parallel_loop_access !6
// CHECK: llvm.loop !6
}

void test_linear() {
}

// test()
// CHECK: !0 = metadata !{metadata !0}
// test1()
// CHECK: !1 = metadata !{metadata !1}
// test_vectorlength()
// CHECK: !2 = metadata !{metadata !2, metadata !3}
// CHECK: !3 = metadata !{metadata !"llvm.vectorizer.width", i32 4}
// CHECK: !4 = metadata !{metadata !4, metadata !5}
// CHECK: !5 = metadata !{metadata !"llvm.vectorizer.width", i32 8}
// CHECK: !6 = metadata !{metadata !6, metadata !7}
// CHECK: !7 = metadata !{metadata !"llvm.vectorizer.width", i32 16}
