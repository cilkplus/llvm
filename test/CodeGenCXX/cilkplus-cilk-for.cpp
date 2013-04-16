// RUN: %clang_cc1 -std=c++11 -fcilkplus -fcxx-exceptions -fexceptions -emit-llvm %s -o %t
// RUN: FileCheck -input-file=%t -check-prefix=CHECK1 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK2 %s
//
struct Bool {
  ~Bool();
  operator bool();
};
struct T { };
Bool operator<(int, const T&);
int operator-(const T&, int);

void test_cond() {
  _Cilk_for (int i = 0; i < T(); i++) { }
  // CHECK1: [[Cond:%[a-zA-Z0-9_]*]] = invoke {{.*}} i1 @{{.*}}BoolcvbEv
  //
  // CHECK1: call void @{{.*}}BoolD1Ev
  // CHECK1: br i1 [[Cond]]
  //
  // CHECK1: call void @__cilkrts_cilk_for_32
  // CHECK1: br
  //
  // CHECK1: landingpad
  // CHECK1: call void @{{.*}}BoolD1Ev
}

struct Int {
  Int(int);
  ~Int();
  operator int ();
};

void test_init() {
  _Cilk_for (int i = Int(19); i < 10; i++) { }
  // CHECK1: call void @{{.*}}IntC1Ei({{.*}}, i32 19)
  // CHECK1: [[Init:%[a-zA-Z0-9_]*]] = invoke i32 @{{.*}}IntcviEv
  //
  // CHECK1: call void @{{.*}}IntD1Ev
  // CHECK1: store i32
  // CHECK1: br i1
  //
  // CHECK1: call void @__cilkrts_cilk_for_32
  // CHECK1: br
  //
  // CHECK1: landingpad
  // CHECK1: call void @{{.*}}IntD1Ev
}

void test_increment() {
  _Cilk_for (int i = 0; i < 10000; i += Int(2012)) {
    extern void anchor() throw();
    anchor();
  }
  // CHECK2: call void @_ZN3IntC1Ei({{.*}}, i32 2012)
  // CHECK2-NEXT: invoke i32 @_ZN3IntcviEv
  // CHECK2-NOT: call @_ZN14test_increment6anchorEv
  //
  // Normal:
  // CHECK2: call void @_ZN3IntD1Ev
  //
  // Excepting:
  // CHECK2: call void @_ZN3IntD1Ev
  //
  // CHECK2: ret void
}

void test_cilk_for_body() {
  // Confirm this compiles; do not check the generated code.
  _Cilk_for (int i = 0; i < 10; i++) {
    try { throw 2013; } catch (...) { }
  }
}
