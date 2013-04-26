// RUN: %clang_cc1 -fcilkplus -ast-dump -ast-dump-filter test_loop_count %s | FileCheck -strict-whitespace %s

class Foo {
public:
  Foo(int);
  ~Foo();
  operator int();
private:
  int Val;
};

int func(Foo);

void test_loop_count() {
  // These tests are check if the loop count expression starts with a
  // 'ExprWithCleanups' node.

  _Cilk_for(int i = 0; i < Foo(7); i += 4);
  // CHECK:      CilkForStmt
  // CHECK:        NullStmt
  // CHECK-NEXT: ExprWithCleanups

  _Cilk_for(int i = 13; i >= 4; i -= func(Foo(4)));
  //for(int i = 13; i >= 4; i -= func(Foo(4)));
  // CHECK:      CilkForStmt
  // CHECK:        NullStmt
  // CHECK-NEXT: ExprWithCleanups

  _Cilk_for(int i = Foo(13); i >= 4; i -= 4);
  // CHECK:      CilkForStmt
  // CHECK:        NullStmt
  // CHECK-NOT: ExprWithCleanups
  // CHECK-NEXT: ImplicitCastExpr

}
