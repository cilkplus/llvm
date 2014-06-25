// RUN: %clang_cc1 -fsyntax-only -ferror-limit 0 -verify %s

namespace X {
  int x;
};

struct A {
  int a;
  void bar() {
    int s;
    #pragma simd linear(s:sizeof(int))
    for (int i = 0; i < 256; i++);
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
  double d;
  _Complex double cd = 0;
  A a;
  a.a = 2;

  /* expected-error@+1 {{reduction operator min requires arithmetic type}} */
  #pragma simd reduction (min:cd)
  for (int i = 0; i < 16; ++i) {cd = 1.234;};
  /* expected-error@+1 {{invalid operands to binary expression ('double' and 'double')}} */
  #pragma simd reduction (^:d)
  for (int i = 0; i < 16; ++i) {d = d + 7.89;} ;

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
