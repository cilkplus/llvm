// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o - | FileCheck %s

int foo(void);

void test() {
  int a = _Cilk_spawn foo();
  // CHECK: alloca
  // CHECK: call i{{[0-9]*}} @foo()
  // CHECK: store
  a = _Cilk_spawn foo();
  // CHECK: call i{{[0-9]*}} @foo()
  // CHECK: store
  _Cilk_spawn foo();
  // CHECK: call i{{[0-9]*}} @foo()
  _Cilk_sync;
}

void bar(_Complex float);

void test_bar() {
  _Complex float b = 0.0f;
  // CHECK: define void @test_bar()
  // CHECK: call void @bar(<2 x float>
  _Cilk_spawn bar(b);
}
