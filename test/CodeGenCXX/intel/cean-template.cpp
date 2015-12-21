// RUN: %clang_cc1 -std=c++11 -DCXX11 -triple x86_64-pc-linux-gnu -emit-llvm -fcilkplus -fms-compatibility %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++11 -DGNU -triple x86_64-pc-linux-gnu -emit-llvm -fcilkplus -fms-compatibility %s -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++11 -fms-extensions -DMS -triple x86_64-pc-win32 -emit-llvm -fcilkplus -fms-compatibility %s -o - | FileCheck %s
// Example from cq371510 (regCpp/cq161918).
// REQUIRES: cilkplus

#ifdef GNU
# define ATTR(x)  __attribute__((x))
#endif
#ifdef MS
# define ATTR(x)  __declspec(x)
#endif
#ifndef ATTR
# define ATTR(x)  [[x]]
#endif

template <typename T> class metasl_vec1_cean;
typedef metasl_vec1_cean<bool> bool_cean;

template <typename T>
class metasl_vec1_cean {
  public:
    ATTR(align(32))  T x[16];
    metasl_vec1_cean(const bool_cean& v) {
      if( v.x[:] ) { x[:] = (T)1.0; } else  { x[:] = (T)0.0; }
      // Check that the if-stmt is wrapped only in 1 loop.
      // CHECK: icmp
      // CHECK-NOT: icmp
    };
};

int foo(const bool_cean &ce) {
  bool_cean bc(ce);
  return 0;
}
