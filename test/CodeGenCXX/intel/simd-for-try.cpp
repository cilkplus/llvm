// RUN: %clang_cc1 -std=c++11 -fcilkplus -fcxx-exceptions -fexceptions -emit-llvm %s -o - | FileCheck %s
// REQUIRES: cilkplus

void extern1();
void extern2();

void test2() {
  try {
    #pragma simd
    for (int i = 0; i < 10; i++) {
      extern1();
    }
    extern2();
  } catch (...) {}

  // CHECK: test2
  // CHECK: call void @{{.*}}extern
  // CHECK: invoke void @{{.*}}extern
}
