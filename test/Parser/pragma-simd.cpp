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


int z;
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
}
