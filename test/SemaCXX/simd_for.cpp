// RUN: %clang_cc1 -std=c++11 -fcilkplus -fsyntax-only -fcxx-exceptions -verify %s
extern int foo();

void test_simd_for_body1() {
  #pragma simd
  for (int i = 0; i < 10; ++i) {
  L1:
    if (foo())
      goto L1; // expected-error {{goto is not allowed within simd for}}
    else
      goto L2; // expected-error {{goto is not allowed within simd for}}
    { }

  L2:
    // no indirect goto
    void *addr = 0;
    {
      addr = &&L2;
      goto *addr; // expected-error {{indirect goto is not allowed within simd for}}
    }

    // no goto inside a lambda.
    []() {
    L3:
     if (foo())
       goto L3; // expected-error {{goto is not allowed within simd for}}
    }();
  }
}

void test_simd_for_body2() {
  #pragma simd
  for (int i = 0; i < 10; ++i) {
    _Cilk_sync; // expected-error {{_Cilk_sync is not allowed within simd for}}
  }

  #pragma simd
  for (int i = 0; i < 10; ++i) {
    _Cilk_spawn foo(); // expected-error {{_Cilk_spawn is not allowed within simd for}}
  }

  #pragma simd
  for (int i = 0; i < 10; ++i) {  // expected-note {{outer simd for here}}
    #pragma simd
    for (int j = 0; j < 10; ++j); // expected-error {{nested simd for not supported}}
  }
}

void test_simd_for_body3() {
  #pragma simd
  for (int i = 0; i < 10; ++i) {
    try {} catch (...) {} // expected-error {{try is not allowed within simd for}}
  }

  #pragma simd
  for (int i = 0; i < 10; ++i) {
    if (foo()) throw i; // expected-error {{throw is not allowed within simd for}}
  }
}

struct A { A(); ~A(); };
struct B { B(); };
A makeA();
B makeB();

void test_simd_for_body4() {
  A a0;
  B b0;
  #pragma simd
  for (int i = 0; i < 10; ++i) {
    const A &a1 = a0;      // OK
    const A &a2 = makeA(); // expected-error {{non-static variable with a non-trivial destructor is not allowed within simd for}}
    A &&a3 = makeA();      // expected-error {{non-static variable with a non-trivial destructor is not allowed within simd for}}
    A *a4;                 // OK
    A  a5;                 // expected-error {{non-static variable with a non-trivial destructor is not allowed within simd for}}

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
}

void test_simd_for_body5() {
  #pragma simd
  for (int i = 0; i < 10; ++i) {
    __builtin_setjmp(0);      // expected-error{{__builtin_setjmp is not allowed within simd for}}
    __builtin_longjmp(0, 1);  // expected-error{{__builtin_longjmp is not allowed within simd for}}
  }
}
