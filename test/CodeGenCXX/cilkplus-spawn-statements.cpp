// RUN: %clang_cc1 -std=c++11 -fcilkplus -emit-llvm %s -o %t
// RUN: FileCheck -check-prefix=CHECK-CILK1 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK2 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK3 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK4 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK5 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK6 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK7 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK8 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK9 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK10 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK11 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK12 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK13 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK14 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK15 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK16 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK17 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK18 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK19 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK20 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK21 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK22 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK23 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK24 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK25 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK26 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK27 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK28 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK29 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK30 --input-file=%t %s
// RUN: FileCheck -check-prefix=CHECK-CILK31 --input-file=%t %s

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

// CHECK-CILK1: define internal void @[[HelperName]]({{.*}}) #[[AttributeNum:[0-9]+]]
// CHECK-CILK1: %{{.*}} = alloca %__cilkrts_stack_frame

// CHECK-CILK1: call void @_ZN3FooC1Ev(%struct.Foo*
// CHECK-CILK1-NEXT: call void @__cilk_helper_prologue(%__cilkrts_stack_frame*
// CHECK-CILK1-NEXT: {{call|invoke}} {{.*}} @_Z1f3Foo(%struct.Foo*

// CHECK-CILK1: call void @_ZN3FooD1Ev(%struct.Foo*

// CHECK-CILK1: call void @__cilk_helper_epilogue(%__cilkrts_stack_frame*
// CHECK-CILK1-NEXT: ret void

// CHECK-CILK1: attributes #[[AttributeNum]] = {{.*}} noinline

//-----------------------------------------------------------------------------

// Make sure that "x" is initialized after the spawn point
void test2() {
  // CHECK-CILK2: define void @_Z5test2v()
  // CHECK-CILK2: call void @[[HelperName:.+cilk_spawn_helper.*]](
  int x = _Cilk_spawn f(param1 + param2);
}

// CHECK-CILK2: define internal void @[[HelperName]]({{.*}})

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

// CHECK-CILK3: define internal void @[[HelperName]]({{.*}})

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

// CHECK-CILK4: define internal void @[[HelperName]]({{.*}})

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

// CHECK-CILK5: define internal void @[[HelperName]]({{.*}})

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

// CHECK-CILK7: define internal void @[[HelperName]]({{.*}})

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

// CHECK-CILK8: define internal void @[[HelperName]]({{.*}})
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

// CHECK-CILK9: define internal void @[[HelperName]]({{.*}})
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

// CHECK-CILK10: define internal void @[[HelperName]]({{.*}})
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

namespace capture_array_by_value {
  template <typename T>
  void touch(const T *) { }

  template <typename T>
  void touch(T) { }

  template <typename T, unsigned n>
  void invoke_ptr() {
    T array[n] = {0, 1, 2, 3, 4, 5};
    _Cilk_spawn [=]() { touch(array); }();
  }

  template <typename T, unsigned n>
  void invoke_val() {
    T array[n] = {0, 1, 2, 3, 4, 5};
    _Cilk_spawn [=]() { touch(array[5]); }();
  }

  template <typename T, unsigned m, unsigned n>
  void invoke_multi() {
    T array[m][n] = {0, 1, 2, 3};
    _Cilk_spawn [=]() { touch(array[1][n - 1]); }();
  }

  void test() {
    struct S { int v; };

    invoke_ptr<float, 17>();
    invoke_ptr<S, 19>();

    invoke_val<float, 21>();
    invoke_val<S, 23>();

    invoke_multi<float, 3, 5>();
    invoke_multi<S, 11, 7>();

    // CHECK-CILK12: %struct.anon{{.*}} = type { [17 x float]* }
    // CHECK-CILK12: %struct.anon{{.*}} = type { [19 x %struct.S]* }
    // CHECK-CILK12: %struct.anon{{.*}} = type { [21 x float]* }
    // CHECK-CILK12: %struct.anon{{.*}} = type { [23 x %struct.S]* }
    // CHECK-CILK12: %struct.anon{{.*}} = type { [3 x [5 x float]]* }
    // CHECK-CILK12: %struct.anon{{.*}} = type { [11 x [7 x %struct.S]]* }
  }
}

namespace spawn_variable_initialization {

template <typename T, int>
T foo();
template <typename T, int>
T& lfoo();
template <typename T, int>
T&& rfoo();
template <typename T, int>
const T& cfoo();

template <typename T, int x>
void value_from_temp() {
  T t = _Cilk_spawn foo<T, x>();
}

template <typename T, int x>
void value_from_const_ref() {
  T t = _Cilk_spawn cfoo<T, x>();
}

template <typename T, int x>
void rvalue_ref_from_temp() {
  T &&t = _Cilk_spawn foo<T, x>();
}

template <typename T, int x>
void const_ref_from_temp() {
  const T &t = _Cilk_spawn foo<T, x>();
}

template <typename T, int x>
void lvalue_ref_from_lvalue_ref() {
  T &t = _Cilk_spawn lfoo<T, x>();
}

template <typename T, int x>
void rvalue_ref_from_rvalue_ref() {
  T &&t = _Cilk_spawn rfoo<T, x>();
}

template <typename T, int x>
void const_ref_from_const_ref() {
  const T &t = _Cilk_spawn cfoo<T, x>();
}

struct Class {
  Class();
  Class(const Class&);
  ~Class();
};

void test() {
  value_from_temp<float, 1001>();
  // CHECK-CILK13: define {{.*}}value_from_temp{{.*}}1001
  // CHECK-CILK13:   alloca float, align
  // CHECK-CILK13:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK13: define internal void @[[Helper]]
  // CHECK-CILK13:   call float @
  // CHECK-CILK13-NEXT: store float %

  value_from_const_ref<float, 1002>();
  // CHECK-CILK14: define {{.*}}value_from_const_ref{{.*}}1002
  // CHECK-CILK14:   alloca float, align
  // CHECK-CILK14:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK14: define internal void @[[Helper]]
  // CHECK-CILK14:   call float* @
  // CHECK-CILK14-NEXT: load float*
  // CHECK-CILK14-NEXT: store float %

  rvalue_ref_from_temp<float, 1003>();
  // CHECK-CILK15: define {{.*}}rvalue_ref_from_temp{{.*}}1003
  // CHECK-CILK15:   alloca float*, align
  // CHECK-CILK15:   alloca float, align
  // CHECK-CILK15:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK15: define internal void @[[Helper]]
  // CHECK-CILK15-NOT: alloca float
  // CHECK-CILK15:   call float @
  // CHECK-CILK15-NEXT: store float %
  // CHECK-CILK15-NEXT: store float* %

  const_ref_from_temp<float, 1004>();
  // CHECK-CILK16: define {{.*}}const_ref_from_temp{{.*}}1004
  // CHECK-CILK16:   alloca float*, align
  // CHECK-CILK16:   alloca float, align
  // CHECK-CILK16:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK16: define internal void @[[Helper]]
  // CHECK-CILK16-NOT: alloca float
  // CHECK-CILK16:   call float @
  // CHECK-CILK16-NEXT: store float %
  // CHECK-CILK16-NEXT: store float* %

  lvalue_ref_from_lvalue_ref<float, 1005>();
  // CHECK-CILK17: define {{.*}}lvalue_ref_from_lvalue_ref{{.*}}1005
  // CHECK-CILK17:   alloca float*, align
  // CHECK-CILK17:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK17: define internal void @[[Helper]]
  // CHECK-CILK17:   call float* @
  // CHECK-CILK17-NEXT: store float* %

  rvalue_ref_from_rvalue_ref<float, 1006>();
  // CHECK-CILK18: define {{.*}}rvalue_ref_from_rvalue_ref{{.*}}1006
  // CHECK-CILK18:   alloca float*, align
  // CHECK-CILK18:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK18: define internal void @[[Helper]]
  // CHECK-CILK18:   call float* @
  // CHECK-CILK18-NEXT: store float* %

  const_ref_from_const_ref<float, 1007>();
  // CHECK-CILK19: define {{.*}}const_ref_from_const_ref{{.*}}1007
  // CHECK-CILK19:   alloca float*, align
  // CHECK-CILK19:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK19: define internal void @[[Helper]]
  // CHECK-CILK19:   call float* @
  // CHECK-CILK19-NEXT: store float* %


  // With destructor
  value_from_temp<Class, 2001>();
  // CHECK-CILK20: define {{.*}}value_from_temp{{.*}}2001
  // CHECK-CILK20:   alloca %"struct.spawn_variable_initialization::Class", align
  // CHECK-CILK20:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  // CHECK-CILK20-NOT: define
  // CHECK-CILK20:   call void @[[Destructor:.*spawn_variable_initialization.ClassD1Ev]]
  //
  // CHECK-CILK20: define internal void @[[Helper]]
  // CHECK-CILK20-NOT: call void @[[Destructor]]
  // CHECK-CILK20:   ret

  value_from_const_ref<Class, 2002>();
  // CHECK-CILK21: define {{.*}}value_from_const_ref{{.*}}2002
  // CHECK-CILK21:   alloca %"struct.spawn_variable_initialization::Class", align
  // CHECK-CILK21:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  // CHECK-CILK21-NOT: define
  // CHECK-CILK21:   call void @[[Destructor:.*spawn_variable_initialization.ClassD1Ev]]
  //
  // CHECK-CILK21: define internal void @[[Helper]]
  // CHECK-CILK21-NOT: call void @[[Destructor]]
  // CHECK-CILK21:   ret

  rvalue_ref_from_temp<Class, 2003>();
  // CHECK-CILK22: define {{.*}}rvalue_ref_from_temp{{.*}}2003
  // CHECK-CILK22:   alloca %"struct.spawn_variable_initialization::Class"*, align
  // CHECK-CILK22:   [[Tmp:%[0-9a-zA-Z_]*]] = alloca %"struct.spawn_variable_initialization::Class", align
  // CHECK-CILK22:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  // CHECK-CILK22-NOT: define
  // CHECK-CILK22:   call void @[[Destructor:.*spawn_variable_initialization.ClassD1Ev]](%{{.*}}* [[Tmp]])
  // CHECK-CILK22:   ret
  //
  // CHECK-CILK22: define internal void @[[Helper]]
  // CHECK-CILK22-NOT: alloca %"struct.spawn_variable_initialization::Class", align
  // CHECK-CILK22-NOT: call void @[[Destructor]]
  // CHECK-CILK22:   ret

  const_ref_from_temp<Class, 2004>();
  // CHECK-CILK23: define {{.*}}const_ref_from_temp{{.*}}2004
  // CHECK-CILK23:   alloca %"struct.spawn_variable_initialization::Class"*, align
  // CHECK-CILK23:   [[Tmp:%[0-9a-zA-Z_]*]] = alloca %"struct.spawn_variable_initialization::Class", align
  // CHECK-CILK23:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  // CHECK-CILK23-NOT: define
  // CHECK-CILK23:   call void @[[Destructor:.*spawn_variable_initialization.ClassD1Ev]](%{{.*}}* [[Tmp]])
  //
  // CHECK-CILK23: define internal void @[[Helper]]
  // CHECK-CILK23-NOT: alloca %"struct.spawn_variable_initialization::Class", align
  // CHECK-CILK23-NOT: call void @[[Destructor]]
  // CHECK-CILK23:   ret

  lvalue_ref_from_lvalue_ref<Class, 2005>();
  // CHECK-CILK24: define {{.*}}lvalue_ref_from_lvalue_ref{{.*}}2005
  // CHECK-CILK24:   alloca %"struct.spawn_variable_initialization::Class"*, align
  // CHECK-CILK24:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  // CHECK-CILK24-NOT: call void @{{.*}}ClassD
  // CHECK-CILK24:   ret
  //
  // CHECK-CILK24: define internal void @[[Helper]]
  // CHECK-CILK24-NOT: call void @{{.*}}ClassD
  // CHECK-CILK24:   ret

  rvalue_ref_from_rvalue_ref<Class, 2006>();
  // CHECK-CILK25: define {{.*}}rvalue_ref_from_rvalue_ref{{.*}}2006
  // CHECK-CILK25:   alloca %"struct.spawn_variable_initialization::Class"*, align
  // CHECK-CILK25:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  // CHECK-CILK25-NOT: call void @{{.*}}ClassD
  // CHECK-CILK25:   ret
  //
  // CHECK-CILK25: define internal void @[[Helper]]
  // CHECK-CILK25-NOT: call void @{{.*}}ClassD
  // CHECK-CILK25:   ret

  const_ref_from_const_ref<Class, 2007>();
  // CHECK-CILK26: define {{.*}}const_ref_from_const_ref{{.*}}2007
  // CHECK-CILK26:   alloca %"struct.spawn_variable_initialization::Class"*, align
  // CHECK-CILK26:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  // CHECK-CILK26-NOT: call void @{{.*}}ClassD
  // CHECK-CILK26:   ret
  //
  // CHECK-CILK26: define internal void @[[Helper]]
  // CHECK-CILK26-NOT: call void @{{.*}}ClassD
  // CHECK-CILK26:   ret
}

struct Base {
  Base();
  virtual ~Base();
};

struct Derived : public Base {
  Derived();
  virtual ~Derived();
};

struct Class2 {
  Class2(const Class &);
};

void test_implicit_conversions() {
  double d = _Cilk_spawn foo<float, 3001>();
  // CHECK-CILK27: define {{.*}}test_implicit_conversions
  // CHECK-CILK27:   alloca double
  // CHECK-CILK27:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK27: define internal void @[[Helper]]
  // CHECK-CILK27:   call {{.*}}foo{{.*}}3001
  // CHECK-CILK27-NEXT: fpext

  void *p = _Cilk_spawn foo<int*, 3002>();
  // CHECK-CILK28: define {{.*}}test_implicit_conversions
  // CHECK-CILK28:   alloca i8*
  // CHECK-CILK28:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK28: define internal void @[[Helper]]
  // CHECK-CILK28:   call {{.*}}foo{{.*}}3002
  // CHECK-CILK28-NEXT: bitcast {{.*}} to i8*

  const Base &b = _Cilk_spawn foo<Derived, 3003>();
  // CHECK-CILK29: define {{.*}}test_implicit_conversions
  // CHECK-CILK29:   alloca %"struct.spawn_variable_initialization::Base"*
  // CHECK-CILK29:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK29: define internal void @[[Helper]]
  // CHECK-CILK29:   call {{.*}}foo{{.*}}3003
  // CHECK-CILK29-NEXT: bitcast

  Class2 c = _Cilk_spawn foo<Class, 3004>();
  // CHECK-CILK30: define {{.*}}test_implicit_conversions
  // CHECK-CILK30:   alloca %"struct.spawn_variable_initialization::Class2"
  // CHECK-CILK30:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  //
  // CHECK-CILK30: define internal void @[[Helper]]
  // CHECK-CILK30:   call void @{{.*}}foo{{.*}}3004
  // CHECK-CILK30-NEXT: call {{.*}}Class2C
}

struct DiamondBase { virtual ~DiamondBase(); };
struct DiamondA : public virtual DiamondBase { virtual ~DiamondA(); };
struct DiamondB : public virtual DiamondBase { virtual ~DiamondB(); };
struct DiamondC : public DiamondA, public DiamondB { virtual ~DiamondC(); };

DiamondC make_DiamondC();

void test_diamond_bind_temp() {
  const DiamondBase& a = _Cilk_spawn make_DiamondC();
  // CHECK-CILK31: define {{.*}}test_diamond_bind_temp
  // CHECK-CILK31:   alloca %"struct.spawn_variable_initialization::DiamondBase"*, align
  // CHECK-CILK31:   [[Tmp:%[0-9a-zA-Z_]*]] = alloca %"struct.spawn_variable_initialization::DiamondC", align
  // CHECK-CILK31:   call void @[[Helper:.*__cilk_spawn_helper[^)]*]](
  // CHECK-CILK31-NOT: define
  // CHECK-CILK31:   call void @[[Destructor:.*spawn_variable_initialization.DiamondCD1Ev]](%{{.*}}* [[Tmp]])
  //
  // CHECK-CILK31: define internal void @[[Helper]]
  // CHECK-CILK31-NOT: alloca %"struct.spawn_variable_initialization::Diamond
  // CHECK-CILK31-NOT: call void @[[Destructor]]
  // CHECK-CILK31:   ret
}

}
