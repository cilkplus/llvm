// RUN: %clang_cc1 -fcilkplus -emit-llvm -g %s -o - | FileCheck %s

// Check debug metadata.
void test() {
  _Cilk_for (int i = 0; i < 10; ++i)
    ;
}

// CHECK: metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata ![[T1:[0-9]+]], i32 0, null, null, null}
// CHECK: ![[T1]] = metadata !{null}
// CHECK: metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata ![[T2:[0-9]+]], i32 0, null, null, null}
// CHECK: ![[T2]] = metadata !{null, metadata !{{[0-9]+}}, metadata !{{[0-9]+}}, metadata !{{[0-9]+}}}
