// RUN: %clang_cc1 -std=c++11 -emit-llvm %s -o - | FileCheck %s

void anchor(int);

void test() {
  #pragma simd
  for (int i = 0; i < 10; ++i) ;
// CHECK: define void @_Z4testv(
// CHECK: llvm.mem.parallel_loop_access !0
// CHECK: llvm.loop !0
  anchor(50);
// CHECK: call void @_Z6anchori(i32 50)
}

void test2(int *A, int N) {
  int *IE = A + N;
  #pragma simd
  for (int *I = A; I != IE; ++I)
    *I *= 2;
// CHECK: define void @_Z5test2Pii
// CHECK: llvm.mem.parallel_loop_access !1
// CHECK: llvm.loop !1

  anchor(60);
// CHECK: call void @_Z6anchori(i32 60)
}

constexpr int width(int i) { return 1 << i; }

void test_vectorlength() {
  #pragma simd vectorlength(4)
  for (int i = 0; i < 10; ++i) ;
// CHECK: define void @_Z17test_vectorlengthv
// CHECK: llvm.mem.parallel_loop_access !2
// CHECK: llvm.loop !2

  anchor(101);
// CHECK: call void @_Z6anchori(i32 101)

  const int W = 1 << 3;
  #pragma simd vectorlength(W)
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !4
// CHECK: llvm.loop !4

  anchor(102);
// CHECK: call void @_Z6anchori(i32 102)

  #pragma simd vectorlength(width(4))
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !6
// CHECK: llvm.loop !6

  anchor(103);
// CHECK: call void @_Z6anchori(i32 103)
}

void touch(int);

void test_access_metadata() {
  #pragma simd
  for (int i = 0; i < 10; ++i) {
    anchor(201);
    touch(i);
    anchor(202);
  }
  anchor(203);
  // CHECK: define void @_Z20test_access_metadatav
  // CHECK: call void @_Z6anchori(i32 201)
  // CHECK: llvm.mem.parallel_loop_access !8
  // CHECK: call void @_Z6anchori(i32 202)
  // CHECK: !llvm.loop !8
  // CHECK: call void @_Z6anchori(i32 203)
}

void test_private_variables(int n) {
  float a = 1.0f;
  #pragma simd private(a)
  for (int i = 0; i < n; ++i) {
    anchor(301);
    touch(a);
    anchor(302);
  }
  // CHECK: call void @_Z6anchori(i32 301)
  // CHECK: load float*
  // CHECK-NOT: getelementptr
  // CHECK: call void @_Z5touchi
  // CHECK: call void @_Z6anchori(i32 302)
}

void test_continue() {
  #pragma simd
  for (int i = 0; i < 10; ++i) {
    anchor(401);
    if (i == 5) continue;
    anchor(402);

    if (true) continue;
    anchor(403);
  }
  anchor(410);
  // CHECK: call void @_Z6anchori(i32 401)
  // CHECK: icmp eq i32
  // CHECK: br label [[CONTINUE_BLOCK:%[_A-Za-z0-9]+]]
  // CHECK: call void @_Z6anchori(i32 402)
  // CHECK-NEXT: br label [[CONTINUE_BLOCK]]
  // CHECK-NOT: call void @_Z6anchori(i32 403)
  // CHECK: call void @_Z6anchori(i32 410)
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
// CHECK: !8 = metadata !{metadata !8}
