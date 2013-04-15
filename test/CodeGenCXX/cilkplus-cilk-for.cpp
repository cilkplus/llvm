// RUN: %clang_cc1 -std=c++11 -fcilkplus -fexceptions -emit-llvm %s -o - | FileCheck %s

struct Bool {
  ~Bool();
  operator bool();
};
struct T { };
Bool operator<(int, const T&);
int operator-(const T&, int);

void test_cond() {
  _Cilk_for (int i = 0; i < T(); i++) { }
  // CHECK: [[Cond:%[a-zA-Z0-9_]*]] = invoke {{.*}} i1 @{{.*}}BoolcvbEv
  //
  // CHECK: call void @{{.*}}BoolD1Ev
  // CHECK: br i1 [[Cond]]
  //
  // CHECK: call void @__cilkrts_cilk_for_32
  // CHECK: br
  //
  // CHECK: landingpad
  // CHECK: call void @{{.*}}BoolD1Ev
}

struct Int {
  Int(int);
  ~Int();
  operator int ();
};

void test_init() {
  _Cilk_for (int i = Int(19); i < 10; i++) { }
  // CHECK: call void @{{.*}}IntC1Ei({{.*}}, i32 19)
  // CHECK: [[Init:%[a-zA-Z0-9_]*]] = invoke i32 @{{.*}}IntcviEv
  //
  // CHECK: call void @{{.*}}IntD1Ev
  // CHECK: store i32
  // CHECK: br i1
  //
  // CHECK: call void @__cilkrts_cilk_for_32
  // CHECK: br
  //
  // CHECK: landingpad
  // CHECK: call void @{{.*}}IntD1Ev
}
