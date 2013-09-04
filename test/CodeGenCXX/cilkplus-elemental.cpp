// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++11 -fcilkplus -emit-llvm %s -o - | FileCheck %s

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

// CHECK: define {{.*}} void @_ZGVxM2v__ZN1XC1Ev
// CHECK: define {{.*}} void @_ZGVxN2v__ZN1XC1Ev
//
// CHECK: define {{.*}} void @_ZGVxN2v__ZN1XC2Ev
//
// CHECK: define {{.*}} void @_ZGVxN2v__ZN1XD1Ev
//
// CHECK: define {{.*}} void @_ZGVxN2v__ZN1XD2Ev
//
// CHECK: define {{.*}} <4 x i32> @_ZGVxN4v__ZN1X2f0Ev
//
// CHECK: define {{.*}} <4 x i32> @_ZGVxN4vv__ZN1X2f1Ei
//
// CHECK: define {{.*}} <4 x i32> @_ZGVxN4vvv__ZN1X2f2Eii
//
// CHECK: define {{.*}} <4 x i32> @_ZGVxN4vv__ZN1X2sfEii

struct Base {
  __attribute__((vector)) virtual int foo();
};

int Base::foo() { return 0; }

struct Derived : public Base {
  __attribute__((vector)) virtual int foo();
};

int Derived::foo() { return 1; }

// CHECK: define {{.*}} <4 x i32> @_ZGVxN4v__ZN4Base3fooEv
// CHECK: define {{.*}} <4 x i32> @_ZGVxN4v__ZN7Derived3fooEv

__attribute__((vector)) void func1(char) { }
// CHECK: define {{.*}} void @_ZGVxN16v__Z5func1c

__attribute__((vector)) void func2(char&) { }
// CHECK: define {{.*}} void @_ZGVxN2v__Z5func2Rc

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

// CHECK: declare {{.*}} @_ZGVxN2v__ZN20ns_check_declaration1X10called_memEv
// CHECK-NOT: declare {{.*}} @_ZGVxN2v__ZN20ns_check_declaration1X14not_called_memEv
// CHECK: declare {{.*}} @_ZGVxN4__ZN20ns_check_declaration1X10static_memEv

} // namespace ns_check_declaration

namespace characteristic_type {

template <typename T>
__attribute__((vector))
T simple_return();

void test1() {
  simple_return<char>();
  simple_return<short>();
  simple_return<int>();
  simple_return<long>();
  simple_return<float>();
  simple_return<double>();
  simple_return<void *>();
}

// CHECK: declare {{.*}} <16 x i8> @_ZGVxM16__ZN19characteristic_type13simple_returnIcEET_v
// CHECK: declare {{.*}} <8 x i16> @_ZGVxM8__ZN19characteristic_type13simple_returnIsEET_v
// CHECK: declare {{.*}} <4 x i32> @_ZGVxM4__ZN19characteristic_type13simple_returnIiEET_v
// CHECK: declare {{.*}} <2 x i64> @_ZGVxM2__ZN19characteristic_type13simple_returnIlEET_v
// CHECK: declare {{.*}} <4 x float> @_ZGVxM4__ZN19characteristic_type13simple_returnIfEET_v
// CHECK: declare {{.*}} <2 x double> @_ZGVxM2__ZN19characteristic_type13simple_returnIdEET_v
// CHECK: declare {{.*}} <2 x i8*> @_ZGVxM2__ZN19characteristic_type13simple_returnIPvEET_v

template <typename... Ts>
__attribute__((vector))
void void_return(Ts... args);

void test2() {
  void_return();
  void_return('a');
  void_return('a', 1.0f);
}

// CHECK: declare {{.*}} void @_ZGVxM4__ZN19characteristic_type11void_returnIJEEEvDpT_(<4 x i32>
// CHECK: declare {{.*}} void @_ZGVxM16v__ZN19characteristic_type11void_returnIJcEEEvDpT_(<16 x i8>
// CHECK: declare {{.*}} void @_ZGVxM16vv__ZN19characteristic_type11void_returnIJcfEEEvDpT_(<16 x i8>

template <typename... Ts>
__attribute__((vector(uniform(x))))
void uniform(char x, Ts... args);

template <typename... Ts>
__attribute__((vector(linear(x))))
void linear(char x, Ts... args);

void test3() {
  uniform('a');
  uniform('a', 'a');
  uniform('a', 1.0);
  uniform('a', nullptr);

  linear('a');
  linear('a', 'a');
  linear('a', 1.0);
  linear('a', nullptr);
}

// CHECK: declare {{.*}} void @_ZGVxM4u__ZN19characteristic_type7uniformIJEEEvcDpT_
// CHECK: declare {{.*}} void @_ZGVxM16uv__ZN19characteristic_type7uniformIJcEEEvcDpT_
// CHECK: declare {{.*}} void @_ZGVxM2uv__ZN19characteristic_type7uniformIJdEEEvcDpT_
// CHECK: declare {{.*}} void @_ZGVxM2uv__ZN19characteristic_type7uniformIJDnEEEvcDpT_
//
// CHECK: declare {{.*}} void @_ZGVxM4l__ZN19characteristic_type6linearIJEEEvcDpT_
// CHECK: declare {{.*}} void @_ZGVxM16lv__ZN19characteristic_type6linearIJcEEEvcDpT_
// CHECK: declare {{.*}} void @_ZGVxM2lv__ZN19characteristic_type6linearIJdEEEvcDpT_
// CHECK: declare {{.*}} void @_ZGVxM2lv__ZN19characteristic_type6linearIJDnEEEvcDpT_

} // namespace characteristic_type

namespace uniform_linear_this {

struct S {
  __attribute__((vector(uniform(this))))
  float m1();

  __attribute__((vector(linear(this))))
  float m2();

  __attribute__((vector(linear(this:7))))
  float m3();

  __attribute__((vector(linear(this:a), uniform(a))))
  float m4(int a);
};

void test(S *p) {
  p->m1();
  p->m2();
  p->m3();
  p->m4(4);
}

// CHECK: declare {{.*}} <4 x float> @_ZGVxN4u__ZN19uniform_linear_this1S2m1Ev
// CHECK: declare {{.*}} <4 x float> @_ZGVxN4l__ZN19uniform_linear_this1S2m2Ev
// CHECK: declare {{.*}} <4 x float> @_ZGVxN4l7__ZN19uniform_linear_this1S2m3Ev
// CHECK: declare {{.*}} <4 x float> @_ZGVxN4s1u__ZN19uniform_linear_this1S2m4Ei

} // namespace uniform_linear_this
