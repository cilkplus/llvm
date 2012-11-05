// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK1 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK2 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK3 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK4 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK5 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK6 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK7 %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK8 %s


/*
 *  A _Cilk_spawn can only appear in 3 places
 *    as the *entire* body of an expression statement
 *      (covered by Test 1, 2, 6, 10)
 *
 *    as the *entire* right hand side of an assignment expression
 *      (covered by Test 4 and 5)
 *
 *    as the entire initializer-clause in a simple declaration
 *      (covered by Test 3, 7, 8, and 9)
 *
 *  Thus we have covered all possible cases for _Cilk_spawn
 */

int global = 0;

struct Foo {
  int x;
  float y;

  Foo() : x(10), y(11.0f) {};
  ~Foo() {global++;}
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

  // CHECK-CILK1: call void @__cilk_spawn_helper[[HelperNum:[0-9]*]](%struct.Foo* {{.*}})
  // CHECK-CILK1-NEXT: br label %{{.*}}
  _Cilk_spawn f(Foo());

  // CHECK-CILK1: call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %__cilkrts_sf)
  // CHECK-CILK1-NEXT: ret void
}

// CHECK-CILK1: define internal void @__cilk_spawn_helper[[HelperNum]](%struct.Foo*{{.*}}) noinline
// CHECK-CILK1: %{{.*}} = alloca %__cilkrts_stack_frame

// CHECK-CILK1: call void @__cilk_helper_epilogue(%__cilkrts_stack_frame*
// CHECK-CILK1-NEXT: ret void

// CHECK-CILK1: call void @_ZN3FooC1Ev(%struct.Foo*
// CHECK-CILK1-NEXT: call void @__cilk_helper_prologue(%__cilkrts_stack_frame*
// CHECK-CILK1-NEXT: call {{.*}} @_Z1f3Foo(%struct.Foo*
// CHECK-CILK1-NEXT: call void @_ZN3FooD1Ev(%struct.Foo*

//-----------------------------------------------------------------------------

// Make sure that "x" is initialized after the spawn point
void test2() {
  // CHECK-CILK2: define void @_Z5test2v()
  // CHECK-CILK2: call void @__cilk_spawn_helper[[HelperNum:[0-9]*]](%struct.Foo*{{.*}}, i32*{{.*}})
  int x = _Cilk_spawn f(param1 + param2);
}

// CHECK-CILK2: define internal void @__cilk_spawn_helper[[HelperNum]](%struct.Foo*{{.*}}, i32*{{.*}}) noinline

// CHECK-CILK2: call void @_ZplRK3FooS1_
// CHECK-CILK2-NEXT: call void @__cilk_helper_prologue
// CHECK-CILK2-NEXT: call {{.*}} @_Z1f3Foo
// CHECK-CILK2-NEXT: call void @_ZN3FooD1Ev
// CHECK-CILK2-NEXT: store {{.*}} %{{.*}}

//-----------------------------------------------------------------------------

// Make sure that "y" is assigned in the child
void test3() {
  // CHECK-CILK3: define void @_Z5test3v()
  // CHECK-CILK3: call void @__cilk_spawn_helper[[HelperNum:[0-9]*]](%struct.Foo*{{.*}}, i32*{{.*}})
  int y;
  y = _Cilk_spawn f(param1 + param2);
}

// CHECK-CILK3: define internal void @__cilk_spawn_helper[[HelperNum]](%struct.Foo*{{.*}}, i32*{{.*}}) noinline

// CHECK-CILK3: call void @_ZplRK3FooS1_
// CHECK-CILK3-NEXT: call void @__cilk_helper_prologue
// CHECK-CILK3-NEXT: call {{.*}} @_Z1f3Foo
// CHECK-CILK3-NEXT: store {{.*}} %{{.*}}
// CHECK-CILK3-NEXT: call void @_ZN3FooD1Ev

//-----------------------------------------------------------------------------

// Make sure conversion from int to float takes place
// in the child
void test4() {
  // CHECK-CILK4: define void @_Z5test4v()
  // CHECK-CILK4: call void @__cilk_spawn_helper[[HelperNum:[0-9]*]]
  float b = _Cilk_spawn i();
}

// CHECK-CILK4: define internal void @__cilk_spawn_helper[[HelperNum]](float*{{.*}}) noinline {

// CHECK-CILK4: call void @__cilk_helper_prologue
// CHECK-CILK4-NEXT: call {{.*}} @_Z1iv()
// CHECK-CILK4-NEXT: sitofp
// CHECK-CILK4-NEXT: store float

//-----------------------------------------------------------------------------

// Test for assigning to non-scalars. Make sure that "a"
// is assigned in the child
void test5() {
  // CHECK-CILK5: define void @_Z5test5v()
  // CHECK-CILK5: call void @__cilk_spawn_helper[[HelperNum:[0-9]*]](%struct.Bar*{{.*}})
  Bar a = _Cilk_spawn g();
}

// CHECK-CILK5: define internal void @__cilk_spawn_helper[[HelperNum]](%struct.Bar*{{.*}}) noinline

// CHECK-CILK5: call void @__cilk_helper_prologue
// CHECK-CILK5-NEXT: call {{.*}} @_Z1gv()
// CHECK-CILK5-NEXT: bitcast
// CHECK-CILK5-NEXT: store

//-----------------------------------------------------------------------------

// Test for multiple spawns in one function
void test6() {
  // CHECK-CILK6: define void @_Z5test6v()

  // CHECK-CILK6: call void @__cilk_spawn_helper[[HelperNum:[0-9]*]]
  int a = _Cilk_spawn i();

  // CHECK-CILK6: call void @__cilk_spawn_helper[[HelperNum:[0-9]*]]
  int b = _Cilk_spawn i();

  // CHECK-CILK6: call void @__cilk_spawn_helper[[HelperNum:[0-9]*]]
  _Cilk_spawn f(Foo());

  // CHECK-CILK6: call void @__cilk_spawn_helper[[HelperNum:[0-9]*]]
  _Cilk_spawn g();
}

//-----------------------------------------------------------------------------

// Make sure calculating the index of "z" and construction
// of Foo() is done before the spawn point. Also make sure
// destruction of Foo() and assignment to "z" happens in the child
void test7() {
  // CHECK-CILK7: define void @_Z5test7v()
  int z[10];

  // CHECK-CILK7: call void @__cilk_spawn_helper[[HelperNum:[0-9]*]]
  z[i()] = _Cilk_spawn f(Foo());
}

// CHECK-CILK7: define internal void @__cilk_spawn_helper[[HelperNum]](%struct.Foo*{{.*}}, [10 x i32]*{{.*}}) noinline

// This is what the IR should be. Re-enable these checks when the correct IR is generated
// The issue is that the call to i() happens after the spawn point and it should happen
// before instead. See Defect 14970
// CHECK-CILK7-DISABLED: call {{.*}} @_Z1iv()
// CHECK-CILK7-NEXT-DISABLED: call void @_ZN3FooC1Ev
// CHECK-CILK7-NEXT-DISABLED: call void @__cilk_helper_prologue
// CHECK-CILK7-NEXT-DISABLED: call {{.*}} @_Z1f3Foo
// CHECK-CILK7-DISABLED: store
// CHECK-CILK7-NEXT-DISABLED: call void @_ZN3FooD1Ev

//-----------------------------------------------------------------------------

// Make sure lambda variable is handled correctly
// along with use of temp variable
void test8() {
  // CHECK-CILK8: define void @_Z5test8v()
  auto test_lambda = [](Foo shouldBeDestructed) {
      return 3;
  };
  //int d = _Cilk_spawn test_lambda(Foo());
}

// Currently we can't handle spawning lambda expressions. Add checks
// when the correct IR is generated. See Defect 15082
