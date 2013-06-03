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

template <typename T>
void test_lcv_type() {
  int i = 0;
  // expected-error@+2 {{loop control variable shall have an integer or pointer type, 'T [10]' type here}}
  #pragma simd
  for (T a[10] = {0}; i < 10; ++i);

  // expected-error@+2 {{loop control variable shall have an integer or pointer type, 'int [10]' type here}}
  #pragma simd
  for (int a[10] = {0}; i < 10; ++i);

  #pragma simd
  for (T j = 0; j < 10; ++j); // OK

  extern T* get();
  #pragma simd
  for (T *p = 0; p < get(); ++p); // OK

  T k = 0;
  #pragma simd
  for (decltype(k) i = 0; i < 10; ++i); // OK

  // expected-error@+2 {{loop control variable shall have an integer or pointer type, 'decltype(k) &' type here}}
  #pragma simd
  for (decltype(k)& i = 0; i < 10; ++i);
}

void test_init() {
  int j = 0;

  #pragma simd
  for (auto i = 0; i < 10; ++i); // OK

  // expected-error@+2 {{use of undeclared identifier 'bar'}}
  #pragma simd
  for (auto x = bar(); j < 10; j++);

  #pragma simd
  for (decltype(j) i = 0; i < 10; ++i); // OK

  // expected-error@+2 {{loop control variable shall have an integer or pointer type, 'int &' type here}}
  #pragma simd
  for (int &i = j; i < 10; ++i);

  // expected-error@+2 {{loop control variable shall have an integer or pointer type, 'int &&' type here}}
  #pragma simd
  for (int &&i = 0; i < 10; ++i);
}

template <typename T>
void test_vectorlengthfor_template() {
  #pragma simd vectorlengthfor(T)
  for (int i = 0; i < 10; ++i);
}

template <typename T>
void test_vectorlengthfor_template2(T t) {
  #pragma simd vectorlengthfor(T)
  for (int i = 0; i < 10; ++i);
}


void test_vectorlengthfor() {
  #pragma simd vectorlengthfor(A)
  for (int i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(B)
  for (int i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(decltype(makeB))
  for (int i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(decltype(nullptr))
  for (int i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(int &)
  for (int i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(int &&)
  for (int i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(const int&)
  for (int i = 0; i < 10; ++i);

  #pragma simd vectorlengthfor(auto) // expected-error {{'auto' not allowed here}}
  for (int i = 0; i < 10; ++i);

  test_vectorlengthfor_template<A>();
  test_vectorlengthfor_template<B>();
  A a;
  test_vectorlengthfor_template2<>([&](int, void*) { const A& aa = a; });

  // FIXME: the following should cause an error once template support is added.
  test_vectorlengthfor_template<void>();
}
