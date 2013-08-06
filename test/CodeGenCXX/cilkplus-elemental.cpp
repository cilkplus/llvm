// RUN: %clang_cc1 -triple x86_64-unknown-unknown -fcilkplus -emit-llvm %s -o - | FileCheck %s

struct X {
  __attribute__((vector)) X();
  __attribute__((vector)) ~X();
  __attribute__((vector)) int f0();
  __attribute__((vector)) int f1(int a);
  __attribute__((vector)) int f2(int a, int b);
  __attribute__((vector)) static int sf(int a, int b);
  int v;
};

X::X() { v = 10; }
X::~X() { }
int X::f0() { return v; }
int X::f1(int a) { return a + v; }
int X::f2(int a, int b) { return a + b + v; }
int X::sf(int a, int b) { return a + b; }

// CHECK: define void @_ZGVxM4v__ZN1XC1Ev
// CHECK: define void @_ZGVxN4v__ZN1XC1Ev
//
// CHECK: define void @_ZGVxN4v__ZN1XC2Ev
//
// CHECK: define void @_ZGVxN4v__ZN1XD1Ev
//
// CHECK: define void @_ZGVxN4v__ZN1XD2Ev
//
// CHECK: define <4 x i32> @_ZGVxN4v__ZN1X2f0Ev
//
// CHECK: define <4 x i32> @_ZGVxN4vv__ZN1X2f1Ei
//
// CHECK: define <4 x i32> @_ZGVxN4vvv__ZN1X2f2Eii
//
// CHECK: define <4 x i32> @_ZGVxN4vv__ZN1X2sfEii

struct Base {
  __attribute__((vector)) virtual int foo();
};

int Base::foo() { return 0; }

struct Derived : public Base {
  __attribute__((vector)) virtual int foo();
};

int Derived::foo() { return 1; }

// CHECK: define <4 x i32> @_ZGVxN4v__ZN4Base3fooEv
// CHECK: define <4 x i32> @_ZGVxN4v__ZN7Derived3fooEv

__attribute__((vector)) void func1(char) { }
// CHECK: define void @_ZGVxN16v__Z5func1c

// FIXME: Enable this test.
// __attribute__((vector)) void func2(char&) { }
// define void @_ZGVxN2v__Z5func2Rc

namespace ns_check_declaration {

__attribute__((vector)) float f0(float a, float b);
__attribute__((vector)) float f1(float a, float b);

void caller() {
  f0(1, 2);
}

// CHECK: declare {{.*}} @_ZGVxN4vv__ZN20ns_check_declaration2f0Eff
// CHECK-NOT: declare {{.*}} @_ZGVxN4vv__ZN20ns_check_declaration2f1Eff

void func() {
  extern __attribute__((vector)) float f2(float a, float b);
  f2(1, 2);
}

// CHECK: declare {{.*}} @_ZGVxN4vv__ZN20ns_check_declaration2f2Eff

struct X {
  __attribute__((vector))        void called_mem();
  __attribute__((vector))        void not_called_mem();
  __attribute__((vector)) static void static_mem();
};

void caller_mem(X *x) {
  x->called_mem();
  x->static_mem();
}

// CHECK: declare void @_ZGVxN4v__ZN20ns_check_declaration1X10called_memEv
// CHECK-NOT: declare void @_ZGVxN4v__ZN20ns_check_declaration1X14not_called_memEv
// CHECK: declare void @_ZGVxN4__ZN20ns_check_declaration1X10static_memEv

} // namespace ns_check_declaration
