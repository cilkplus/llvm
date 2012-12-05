// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK1 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK2 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK3 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK4 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK5 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK6 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK7 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK8 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK9 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK10 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK11 %s

/*
 *  A _Cilk_spawn can only appear in 3 places
 *    as the *entire* body of an expression statement
 *      (covered by Test 1 and 6)
 *
 *    as the *entire* right hand side of an assignment expression
 *      (covered by Test 3 and 7)
 *
 *    as the entire initializer-clause in a simple declaration
 *      (covered by Test 2, 4, 5, 6, 8)
 *
 *  Thus we have covered all possible cases for _Cilk_spawn
 */

int global = 0;

int fib(int n) {
  if(n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}

struct Foo {
  int x;
  float y;

  Foo() : x(10), y(11.0f) {};
  ~Foo() {global++;}
  void MemFunc() {
    _Cilk_spawn fib(x);
  }
};

Foo operator+(const Foo& lhs, const Foo& rhs) {
  return Foo();
}


struct Bar {
  float x;
  int y;

  Bar() : x(10.0f), y(11) {}
  ~Bar() = default;
};

Foo param1;
Foo param2;

int f(Foo shouldBeDestructed) {
  return 3;
}

Bar g() {
  return Bar();
}

void h() {
  global++;
}

int i() {
  return 4;
}

//-----------------------------------------------------------------------------

// Make sure Foo() is constructed before spawn point
// and not destructed until after spawn point
void test1() {
  // CHECK-CILK1: define void @_Z5test1v()
  // CHECK-CILK1: %__cilkrts_sf = alloca %__cilkrts_stack_frame
  // CHECK-CILK1: call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %__cilkrts_sf)

  // CHECK-CILK1: call i8* @llvm.frameaddress
  // CHECK-CILK1: call i8* @llvm.stacksave
  // CHECK-CILK1: call i32 @llvm.eh.sjlj.setjmp
  // CHECK-CILK1: br i1 {{%[0-9]+}}, label %{{.*}}, label %{{.*}}
  // CHECK-CILK1: call void @[[HelperName:.+cilk_spawn_helper.*]](
  // CHECK-CILK1-NEXT: br label %{{.*}}
  _Cilk_spawn f(Foo());

  // CHECK-CILK1: call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %__cilkrts_sf)
  // CHECK-CILK1-NEXT: ret void
}

// CHECK-CILK1: define internal void @[[HelperName]]({{.*}}) {{.*noinline.*}}
// CHECK-CILK1: %{{.*}} = alloca %__cilkrts_stack_frame

// CHECK-CILK1: call void @_ZN3FooC1Ev(%struct.Foo*
// CHECK-CILK1-NEXT: call void @__cilk_helper_prologue(%__cilkrts_stack_frame*
// CHECK-CILK1-NEXT: {{call|invoke}} {{.*}} @_Z1f3Foo(%struct.Foo*

// CHECK-CILK1: call void @_ZN3FooD1Ev(%struct.Foo*

// CHECK-CILK1: call void @__cilk_helper_epilogue(%__cilkrts_stack_frame*
// CHECK-CILK1-NEXT: ret void

//-----------------------------------------------------------------------------

// Make sure that "x" is initialized after the spawn point
void test2() {
  // CHECK-CILK2: define void @_Z5test2v()
  // CHECK-CILK2: call void @[[HelperName:.+cilk_spawn_helper.*]](
  int x = _Cilk_spawn f(param1 + param2);
}

// CHECK-CILK2: define internal void @[[HelperName]]({{.*}}) {{.*noinline.*}}

// CHECK-CILK2: call void @_ZplRK3FooS1_
// CHECK-CILK2-NEXT: call void @__cilk_helper_prologue
// CHECK-CILK2-NEXT: {{call|invoke}} {{.*}} @_Z1f3Foo

// CHECK-CILK2-NEXT: call void @_ZN3FooD1Ev
// CHECK-CILK2-NEXT: store i32

//-----------------------------------------------------------------------------

// Make sure that "y" is assigned in the child
void test3() {
  // CHECK-CILK3: define void @_Z5test3v()
  // CHECK-CILK3: call void @[[HelperName:.+cilk_spawn_helper.*]](
  int y;
  y = _Cilk_spawn f(param1 + param2);
}

// CHECK-CILK3: define internal void @[[HelperName]]({{.*}}) {{.*noinline.*}}

// CHECK-CILK3: call void @_ZplRK3FooS1_
// CHECK-CILK3-NEXT: call void @__cilk_helper_prologue
// CHECK-CILK3-NEXT: {{call|invoke}} {{.*}} @_Z1f3Foo

// CHECK-CILK3: store
// CHECK-CILK3-NEXT: call void @_ZN3FooD1Ev

//-----------------------------------------------------------------------------

// Make sure conversion from int to float takes place
// in the child
void test4() {
  // CHECK-CILK4: define void @_Z5test4v()
  // CHECK-CILK4: call void @[[HelperName:.+cilk_spawn_helper.*]](
  float b = _Cilk_spawn i();
}

// CHECK-CILK4: define internal void @[[HelperName]]({{.*}}) {{.*noinline.*}} {

// CHECK-CILK4: call void @__cilk_helper_prologue
// CHECK-CILK4-NEXT: call {{.*}} @_Z1iv()
// CHECK-CILK4-NEXT: sitofp
// CHECK-CILK4-NEXT: store float

//-----------------------------------------------------------------------------

// Test for assigning to non-scalars. Make sure that "a"
// is assigned in the child
void test5() {
  // CHECK-CILK5: define void @_Z5test5v()
  // CHECK-CILK5: call void @[[HelperName:.+cilk_spawn_helper.*]](
  Bar a = _Cilk_spawn g();
}

// CHECK-CILK5: define internal void @[[HelperName]]({{.*}}) {{.*noinline.*}}

// CHECK-CILK5: call void @__cilk_helper_prologue
// CHECK-CILK5-NEXT: call {{.*}} @_Z1gv()
// CHECK-CILK5-NEXT: bitcast
// CHECK-CILK5-NEXT: store

//-----------------------------------------------------------------------------

// Test for multiple spawns in one function
void test6() {
  // CHECK-CILK6: define void @_Z5test6v()

  // CHECK-CILK6: call void @[[HelperName:.+cilk_spawn_helper.*]](
  int a = _Cilk_spawn i();

  // CHECK-CILK6: call void @[[HelperName:.+cilk_spawn_helper.*]](
  int b = _Cilk_spawn i();

  // CHECK-CILK6: call void @[[HelperName:.+cilk_spawn_helper.*]](
  _Cilk_spawn f(Foo());

  // CHECK-CILK6: call void @[[HelperName:.+cilk_spawn_helper.*]](
  _Cilk_spawn g();
}

//-----------------------------------------------------------------------------

// CHECK-CILK7: %[[CaptureStruct:.*]] = type { [10 x i32]* }

// Make sure calculating the index of "z" and construction
// of Foo() is done before the spawn point. Also make sure
// destruction of Foo() and assignment to "z" happens in the child
void test7() {
  // CHECK-CILK7: define void @_Z5test7v()
  int z[10];

  // CHECK-CILK7: call void @[[HelperName:.+cilk_spawn_helper.*]](%[[CaptureStruct]]*
  z[i()] = _Cilk_spawn f(Foo());
}

// CHECK-CILK7: define internal void @[[HelperName]]({{.*}}) {{.*noinline.*}}

// CHECK-CILK7: call {{.*}} @_Z1iv()
// CHECK-CILK7: call void @_ZN3FooC1Ev
// CHECK-CILK7-NEXT: call void @__cilk_helper_prologue
// CHECK-CILK7-NEXT: {{call|invoke}} {{.*}} @_Z1f3Foo

// CHECK-CILK7: store
// CHECK-CILK7-NEXT: call void @_ZN3FooD1Ev

//-----------------------------------------------------------------------------

// Test for handling lambda functions correctly
void test8() {
  // CHECK-CILK8: define void @_Z5test8v()
  auto test_lambda = [](Foo shouldBeDestructed) {
      return 3;
  };

  // CHECK-CILK8: call void @[[HelperName:.+cilk_spawn_helper.*]](
  int d = _Cilk_spawn test_lambda(Foo());
}

// CHECK-CILK8: define internal void @[[HelperName]]({{.*}}) {{.*noinline.*}}
// CHECK-CILK8: call void @_ZN3FooC1Ev
// CHECK-CILK8-NEXT: call void @__cilk_helper_prologue
// CHECK-CILK8-NEXT: {{call|invoke}} i32 @"_ZZ5test8vENK3$_0clE3Foo"

// CHECK-CILK8: call void @_ZN3FooD1Ev
// CHECK-CILK8-NEXT: store

//-----------------------------------------------------------------------------

// Simplest test possible
void test9() {
  // CHECK-CILK9: define void @_Z5test9v()
  // CHECK-CILK9: call void @[[HelperName:.+cilk_spawn_helper.*]](
  _Cilk_spawn h();
}

// CHECK-CILK9: define internal void @[[HelperName]]({{.*}}) {{.*noinline.*}}
// CHECK-CILK9: call void @__cilk_helper_prologue
// CHECK-CILK9-NEXT: call void @_Z1hv

//-----------------------------------------------------------------------------

// CHECK-CILK10: %[[CaptureStruct:.*]] = type { %struct.Foo* }

// Test for _Cilk_spawn inside member function
void test10() {

  Foo x;
  x.MemFunc();
}

// CHECK-CILK10: define linkonce_odr void @_ZN3Foo7MemFuncEv
// CHECK-CILK10: call void @[[HelperName:.+cilk_spawn_helper.*]](%[[CaptureStruct]]*

// CHECK-CILK10: define internal void @[[HelperName]]({{.*}}) {{.*noinline.*}}
// CHECK-CILK10: call void @__cilk_helper_prologue
// CHECK-CILK10-NEXT: call i32 @_Z3fibi

// CHECK-CILK10: store

//-----------------------------------------------------------------------------
namespace scopeinfo_missing_during_instantiation {

  template <typename T>
  void foo(T &x) { }

  template <typename T>
  void bar(T &x) {
    _Cilk_spawn foo(x);
  }
  // CHECK-CILK11: define {{.*}} void @_ZN38scopeinfo_missing_during_instantiation3barIiEEvRT_
  // CHECK-CILK11-NOT: call void @_ZN38scopeinfo_missing_during_instantiation3fooIiEEvRT_
  // CHECK-CILK11: call void @{{.+cilk_spawn_helper.*}}
  void baz() {
    int x = 0;
    bar(x);
  }

}
