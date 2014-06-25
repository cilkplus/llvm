// RUN: %clang_cc1 -fcilkplus -ast-dump %s | FileCheck %s

template <int length>
void test_simd_length() {
// CHECK: test_simd_length
  #pragma simd vectorlength(length)
  for (int i = 0; i < 10; i++) {}
  // CHECK:   TemplateArgument
  // CHECK:       SIMDForStmt
  // CHECK-NEXT:    SIMDAttr
  // CHECK-NEXT:    SIMDLengthAttr
  // CHECK-NEXT:      IntegerLiteral {{.*}} 'int' 2
}

template <typename T>
void test_simd_linear() {
// CHECK: test_simd_linear
  T t;
  #pragma simd linear(t:2)
  for (int i = 0; i < 10; i++) {}
  // CHECK:   TemplateArgument
  // CHECK:       SIMDForStmt
  // CHECK-NEXT:    SIMDAttr
  // CHECK-NEXT:    SIMDLinearAttr
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'int'
  // CHECK-NEXT:      IntegerLiteral {{.*}} 'int' 2
}

template <typename T1, typename T2>
void test_simd_private() {
// CHECK: test_simd_private
  T1 x;
  T2 y;
  int z;
  #pragma simd private(x, y, z)
  for (int i = 0; i < 10; i++) {}
  // CHECK:   TemplateArgument
  // CHECK:       SIMDForStmt
  // CHECK-NEXT:    SIMDAttr
  // CHECK-NEXT:    SIMDPrivateAttr
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'float'
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'long'
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'int'
}

template <typename T1, typename T2>
void test_simd_first_private() {
// CHECK: test_simd_first_private
  T1 x;
  T2 y;
  int z;
  #pragma simd firstprivate(x, y, z)
  for (int i = 0; i < 10; i++) {}
  // CHECK:   TemplateArgument
  // CHECK:       SIMDForStmt
  // CHECK-NEXT:    SIMDAttr
  // CHECK-NEXT:    SIMDFirstPrivateAttr
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'long'
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'float'
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'int'
}

template <typename T1, typename T2>
void test_simd_last_private() {
// CHECK: test_simd_last_private
  T1 x;
  T2 y;
  long z;
  #pragma simd lastprivate(x, y, z)
  for (int i = 0; i < 10; i++) {}
  // CHECK:   TemplateArgument
  // CHECK:       SIMDForStmt
  // CHECK-NEXT:    SIMDAttr
  // CHECK-NEXT:    SIMDLastPrivateAttr
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'double'
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'float'
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'long'
}

template <typename T>
void test_simd_reduction() {
// CHECK: test_simd_reduction
  T x;
  #pragma simd reduction(+:x)
  for (int i = 0; i < 10; i++) {}
  // CHECK:   TemplateArgument
  // CHECK:       SIMDForStmt
  // CHECK-NEXT:    SIMDAttr
  // CHECK-NEXT:    SIMDReductionAttr
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'int'
}

template <typename T1, typename T2, typename T3>
void test_simd_multiple_clauses() {
// CHECK: test_simd_multiple_clauses
  T1 x, x2;
  const T2 y = 4;
  T3 z;
  #pragma simd vectorlength(y) reduction(+:x,x2) lastprivate(z)
  for (int i = 0; i < 10; i++) {}
  // CHECK:   TemplateArgument
  // CHECK:   TemplateArgument
  // CHECK:   TemplateArgument
  // CHECK:       SIMDForStmt
  // CHECK-NEXT:    SIMDAttr
  // CHECK-NEXT:    SIMDLengthAttr
  // CHECK-NEXT:      IntegerLiteral {{.*}} 'const long' 4
  // CHECK-NEXT:    SIMDReductionAttr
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'float'
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'float'
  // CHECK-NEXT:    SIMDLastPrivateAttr
  // CHECK-NEXT:      DeclRefExpr {{.*}} 'double'
}

void test_simd_clauses() {
  test_simd_length<2>();
  test_simd_linear<int>();
  test_simd_private<float, long>();
  test_simd_first_private<long, float>();
  test_simd_last_private<double, float>();
  test_simd_reduction<int>();
  test_simd_multiple_clauses<float, long, double>();
}
