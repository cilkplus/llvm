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

void baz_spawn_spawn_a(void);
void baz_spawn_spawn_b(void);
void test_spawn_spawn() {

  if (foo())
    _Cilk_spawn baz_spawn_spawn_a();
  else
    _Cilk_spawn baz_spawn_spawn_b();
  // CHECK: call void @baz_spawn_spawn_a
  // CHECK-NEXT: call void @__cilk_helper_epilogue
  // CHECK: call void @baz_spawn_spawn_b
  // CHECK-NEXT: call void @__cilk_helper_epilogue
}

void baz_then_spawn_a(void);
void baz_then_spawn_b(void);
void test_then_spawn() {
  if (foo())
    _Cilk_spawn baz_then_spawn_a();
  else
    baz_then_spawn_b();
  // CHECK: call void @baz_then_spawn_b
  // CHECK-NEXT-NOT: call void @__cilk_helper_epilogue
  // CHECK: call void @baz_then_spawn_a
  // CHECK-NEXT: call void @__cilk_helper_epilogue
}

void baz_else_spawn_a(void);
void baz_else_spawn_b(void);
void test_else_spawn() {
  if (foo())
    baz_else_spawn_a();
  else
    _Cilk_spawn baz_else_spawn_b();
  // CHECK: call void @baz_else_spawn_a
  // CHECK-NEXT-NOT: call void @__cilk_helper_epilogue
  // CHECK: call void @baz_else_spawn_b
  // CHECK-NEXT: call void @__cilk_helper_epilogue
}
