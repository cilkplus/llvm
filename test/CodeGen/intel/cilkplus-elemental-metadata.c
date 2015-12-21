// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o - | FileCheck %s
// REQUIRES: cilkplus
// XFAIL: win

#ifdef _WIN32
__declspec(vector(linear(l : m), uniform(m)))
#else
__attribute__((vector(linear(l : m), uniform(m))))
#endif
  int polynom_lu(int *l, int b, int m) {
    *l += 66;
    return 2 * b * b + b - *l;
  }

//CHECK: define {{.+}}polynom_lu(i32* %l, i32 %b, i32 %m) #[[NAME:[0-9]+]] {
//CHECK: attributes #[[NAME]] = {{.+}}"_ZGVxM4s2vu_" "_ZGVxN4s2vu_"
