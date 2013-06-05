// RUN: %clang_cc1 -fcilkplus -ast-dump -ast-dump-filter test_loop_count %s | FileCheck %s

void test_loop_count() {
  // Testing ((first) - (limit)) / -stride
  // Since the loop control variable is 'int', there should be an implicit cast
  // to 'unsigned int'.
  _Cilk_for(int i = 10; i != 0; i--);
  // CHECK:      CilkForStmt
  // CHECK:          NullStmt
  // CHECK-NEXT:   ImplicitCastExpr
  // CHECK-NEXT:     BinaryOperator
  // CHECK-NEXT:       ConditionalOperator
  // CHECK-NEXT:         BinaryOperator
  // CHECK-NEXT:           UnaryOperator
  // CHECK-NEXT:             IntegerLiteral
  // CHECK-NEXT:           IntegerLiteral
  // CHECK-NEXT:         UnaryOperator
  // CHECK-NEXT:           BinaryOperator
  // CHECK-NEXT:             IntegerLiteral
  // CHECK-NEXT:               ImplicitCastExpr
  // CHECK-NEXT:                 DeclRefExpr
  // CHECK-NEXT:         BinaryOperator
  // CHECK-NEXT:           IntegerLiteral
  // CHECK-NEXT:           ImplicitCastExpr
  // CHECK-NEXT:             DeclRefExpr
  // CHECK-NEXT:       ConditionalOperator
  // CHECK-NEXT:         BinaryOperator
  // CHECK-NEXT:           UnaryOperator
  // CHECK-NEXT:             IntegerLiteral
  // CHECK-NEXT:           IntegerLiteral
  // CHECK-NEXT:         UnaryOperator
  // CHECK-NEXT:           UnaryOperator
  // CHECK-NEXT:             IntegerLiteral
  // CHECK-NEXT:         UnaryOperator
  // CHECK-NEXT:           IntegerLiteral

  // Testing ((limit) - (first)) / stride
  // Since the loop control variable is already 'unsigned int', there should not
  // be any implicit cast.
  _Cilk_for(unsigned int i = 0u; i < 10u; ++i);
  // CHECK:      CilkForStmt
  // CHECK:          NullStmt
  // CHECK-NEXT:   BinaryOperator
  // CHECK-NEXT:     BinaryOperator
  // CHECK-NEXT:       BinaryOperator
  // CHECK-NEXT:         BinaryOperator
  // CHECK-NEXT:           IntegerLiteral
  // CHECK-NEXT:           ImplicitCastExpr
  // CHECK-NEXT:             DeclRefExpr
  // CHECK-NEXT:         ImplicitCastExpr
  // CHECK-NEXT:           IntegerLiteral
  // CHECK-NEXT:       ImplicitCastExpr
  // CHECK-NEXT:           IntegerLiteral
  // CHECK-NEXT:     ImplicitCastExpr
  // CHECK-NEXT:       IntegerLiteral

  // Testing ((limit) - (first) + 1) / stride
  // Since the loop control variable is already 'unsigned long long', there
  // should not be any implicit cast.
  _Cilk_for(unsigned long long i = 0ull; i <= 11ull; i += 1);
  // CHECK:      CilkForStmt
  // CHECK:          NullStmt
  // CHECK-NEXT:   BinaryOperator
  // CHECK-NEXT:     BinaryOperator
  // CHECK-NEXT:       BinaryOperator
  // CHECK-NEXT:         IntegerLiteral
  // CHECK-NEXT:         ImplicitCastExpr
  // CHECK-NEXT:           DeclRefExpr
  // CHECK-NEXT:       ImplicitCastExpr
  // CHECK-NEXT:         IntegerLiteral
  // CHECK-NEXT:     ImplicitCastExpr
  // CHECK-NEXT:       IntegerLiteral

}
