// RUN: %clang_cc1 -std=c++11 -fcilkplus -fsyntax-only -fcxx-exceptions -verify -Wno-unused %s
extern int foo();

[[vector]]
void test_elemental_body0() throw() { // expected-error {{exception specifications are not allowed on elemental functions}}
}

[[vector]]
void test_elemental_body1() {
  for (int i = 0; i < 10; ++i) {
  L1:
    if (foo())
      goto L1; // expected-error {{goto is not allowed within an elemental function}}
    else
      goto L2; // expected-error {{goto is not allowed within an elemental function}}
    { }

  L2:
    // no indirect goto
    void *addr = 0;
    {
      addr = &&L2;
      goto *addr; // expected-error {{indirect goto is not allowed within an elemental function}}
    }
  }
}

[[vector]]
void test_elemental_body2() {
  for (int i = 0; i < 10; ++i) {
    _Cilk_sync; // expected-error {{_Cilk_sync is not allowed within an elemental function}}
  }

  for (int i = 0; i < 10; ++i) {
    _Cilk_spawn foo(); // expected-error {{_Cilk_spawn is not allowed within an elemental function}}
  }

  _Cilk_for (int i = 0; i < 10; ++i) { // expected-error {{_Cilk_for is not allowed within an elemental function}}
  }

  // Not forbidden in elemental functions.
  #pragma simd
  for (int i = 0; i < 10; ++i) {
  }
}

[[vector]]
void test_elemental_body3() {
  for (int i = 0; i < 10; ++i) {
    try {} catch (...) {} // expected-error {{try is not allowed within an elemental function}}
  }

  for (int i = 0; i < 10; ++i) {
    if (foo()) throw i; // expected-error {{throw is not allowed within an elemental function}}
  }
}

struct A { A(); ~A(); };
struct B { B(); };
A makeA();
B makeB();

[[vector]]
void test_elemental_body4() {
  A a0;                  // expected-error {{non-static variable with a non-trivial destructor is not allowed within an elemental function}}
  B b0;
  const A &a1 = a0;      // OK
  const A &a2 = makeA(); // expected-error {{non-static variable with a non-trivial destructor is not allowed within an elemental function}}
  A &&a3 = makeA();      // expected-error {{non-static variable with a non-trivial destructor is not allowed within an elemental function}}
  A *a4;                 // OK
  A  a5;                 // expected-error {{non-static variable with a non-trivial destructor is not allowed within an elemental function}}

  static const A &a6 = makeA(); // OK
  static A &&a7 = makeA();      // OK
  static A a8;                  // OK
  extern A a9;                  // OK

  const B &b1 = b0;      // OK
  const B &b2 = makeB(); // OK
  B &&b3 = makeB();      // OK
  B *b4;                 // OK
  B  b5;                 // OK
}

[[vector]]
void test_elemental_body5() {
  __builtin_setjmp(0);      // expected-error{{__builtin_setjmp is not allowed within an elemental function}}
  __builtin_longjmp(0, 1);  // expected-error{{__builtin_longjmp is not allowed within an elemental function}}
}
