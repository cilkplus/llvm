// RUN: %clang_cc1 -fsyntax-only -ferror-limit 0 -verify %s

namespace X {
  int x;
};

struct A {
  int a;
  void bar() {
    #pragma simd linear(X::x)
    for (int i = 0; i < 10; i++) ;
    int j;
    #pragma simd linear(j)
    for (int i = 0; i < 10; i++) ;

    /* expected-error@+1 {{invalid linear step: expected integral constant or variable reference}} */
    #pragma simd linear(j:this->a)
    for (int i = 0; i < 10; i++) ;
    /* expected-error@+1 {{invalid linear variable}} */
    #pragma simd linear(a)
    for (int i = 0; i < 10; i++) ;
  }
};


void foo(int a[]) {
  extern int b[];
  #pragma simd linear(a)
  for (int i = 0; i < 10; i++) ;

  /* expected-error@+1 {{invalid linear variable}} */
  #pragma simd linear(b)
  for (int i = 0; i < 10; i++) ;
}


int z;
const int C0 = 0;
const int C1 = 1;
const int C2 = 2;

void test_linear()
{
  int y;
  char c;
  A a;
  a.a = 2;

  #pragma simd linear(X::x)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd linear(y:z)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd linear(X::x : ::z)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd linear(y,::z, X::x)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd linear(::z)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd linear(y:C1+C2)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd linear(c:y)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd linear(y:c)
  for (int i = 0; i < 10; ++i) ;

  /* expected-error@+1 {{invalid linear variable}} */
  #pragma simd linear(a.a:1)
  for (int i = 0; i < 10; ++i) ;
  /* expected-error@+1 {{invalid linear step: expected integral constant or variable reference}} */
  #pragma simd linear(y:y*z)
  for (int i = 0; i < 10; ++i) ;
  /* expected-error@+1 {{invalid linear step: expected integral constant or variable reference}} */
  #pragma simd linear(y:a.a)
  for (int i = 0; i < 10; ++i) ;

  int ai[10];
  /* expected-error@+1 {{invalid linear variable}} */
  #pragma simd linear(ai)
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{invalid linear variable}} */
  #pragma simd linear(ai[1])
  for (int i = 0; i < 10; ++i);

#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wsource-uses-cilk-plus"
  /* expected-warning@+1 {{linear step is zero}} */
  #pragma simd linear(y:0)
  for (int i = 0; i < 10; ++i);
  /* expected-warning@+1 {{linear step is zero}} */
  #pragma simd linear(y:C0)
  for (int i = 0; i < 10; ++i);
  /* expected-warning@+1 {{linear step is zero}} */
  #pragma simd linear(y:C1-C1)
  for (int i = 0; i < 10; ++i);
#pragma clang diagnostic pop
}
