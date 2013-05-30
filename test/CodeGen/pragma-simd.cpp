// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck %s

void test() {
  #pragma simd
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !0
// CHECK: llvm.mem.parallel_loop_access !0
// CHECK: llvm.loop !0
}

constexpr int width(int i) { return 1 << i; }

void test_vectorlength() {
  #pragma simd vectorlength(4)
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !1
// CHECK: llvm.mem.parallel_loop_access !1
// CHECK: llvm.loop !1

  const int W = 1 << 3;
  #pragma simd vectorlength(W)
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !3
// CHECK: llvm.mem.parallel_loop_access !3
// CHECK: llvm.loop !3

  #pragma simd vectorlength(width(4))
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !5
// CHECK: llvm.mem.parallel_loop_access !5
// CHECK: llvm.loop !5
}

void test_linear() {
}

// test()
// CHECK: !0 = metadata !{metadata !0}
// CHECK: !1 = metadata !{metadata !1, metadata !2}

// test_vectorlength()
// CHECK: !2 = metadata !{metadata !"llvm.vectorizer.width", i32 4}
// CHECK: !3 = metadata !{metadata !3, metadata !4}
// CHECK: !4 = metadata !{metadata !"llvm.vectorizer.width", i32 8}
// CHECK: !5 = metadata !{metadata !5, metadata !6}
// CHECK: !6 = metadata !{metadata !"llvm.vectorizer.width", i32 16}
