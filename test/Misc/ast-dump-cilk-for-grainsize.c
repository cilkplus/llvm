// RUN: %clang_cc1 -fcilkplus -ast-dump -ast-dump-filter test_grainsize %s | FileCheck %s

void test_grainsize() {
  #pragma cilk grainsize = 2
  _Cilk_for(int i = 0; i < 10; ++i);
  // CHECK:    CilkForGrainsizeStmt
  // CHECK-NEXT: IntegerLiteral
  // CHECK-NEXT: CilkForStmt

  unsigned long long gs1 = 12ull;
  #pragma cilk grainsize = gs1
  _Cilk_for(int i = 0; i < 10; ++i);
  // CHECK:    CilkForGrainsizeStmt
  // CHECK-NEXT: ImplicitCastExpr
  // CHECK-NEXT:   ImplicitCastExpr
  // CHECK-NEXT:     DeclRefExpr
  // CHECK-NEXT: CilkForStmt

  int gs2 = 12;
  #pragma cilk grainsize = gs2 / 2
  _Cilk_for(int i = 0; i < 10; ++i);
  // CHECK:    CilkForGrainsizeStmt
  // CHECK-NEXT: BinaryOperator
  // CHECK-NEXT:   ImplicitCastExpr
  // CHECK-NEXT:     DeclRefExpr
  // CHECK-NEXT:   IntegerLiteral
  // CHECK-NEXT: CilkForStmt
}
