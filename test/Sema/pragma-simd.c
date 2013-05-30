// RUN: %clang_cc1 -fsyntax-only -verify %s

int I();
const int C0 = 0;
const int C1 = 1;
const int C2 = 2;
const char CC = (char)32;
const short CS = (short)32;
void test_vectorlength() {
  int i, j;
  short s = 10;
  #pragma simd vectorlength(4)
  for (i = 0; i < 10; ++i) ;
  #pragma simd vectorlength(CC)
  for (i = 0; i < 10; ++i) ;
  #pragma simd vectorlength(CS)
  for (i = 0; i < 10; ++i) ;
  #pragma simd vectorlength(C1)
  for (i = 0; i < 10; ++i) ;
  #pragma simd vectorlength(C2+C2)
  for (i = 0; i < 10; ++i) ;

  /* expected-error@+1 {{invalid vectorlength expression: must be a power of two}} */
  #pragma simd vectorlength(3)
  /* expected-error@+1 {{invalid vectorlength expression: must be a power of two}} */
  #pragma simd vectorlength(C1+C2)
  /* expected-error@+1 {{invalid vectorlength expression: must be an integer constant}} */
  #pragma simd vectorlength(j)
  /* expected-error@+1 {{invalid vectorlength expression: must be an integer constant}} */
  #pragma simd vectorlength(C1+j)
  /* expected-error@+1 {{invalid vectorlength expression: must be an integer constant}} */
  #pragma simd vectorlength(I())
}

struct A { int a; };

void test_linear(int arr[]) {
  int i, j, k = 1;

  struct A a;
  a.a = 2;

  #pragma simd linear(arr)
  for (int i = 0; i < 10; i++) ;
  #pragma simd linear(j)
  for (i = 0; i < 10; ++i) ;
  #pragma simd linear(j:1)
  for (i = 0; i < 10; ++i) ;
  #pragma simd linear(j:C1)
  for (i = 0; i < 10; ++i) ;
  #pragma simd linear(j:C1+C2)
  for (i = 0; i < 10; ++i) ;
  #pragma simd linear(j:k)
  for (i = 0; i < 10; ++i) ;

  extern int arr_b[];
  /* expected-error@+1 {{invalid linear variable}} */
  #pragma simd linear(arr_b)
  for (int i = 0; i < 10; i++) ;
  /* expected-error@+1 {{invalid linear variable}} */
  #pragma simd linear(a.a:1)
  for (int i = 0; i < 10; ++i) ;
  /* expected-error@+1 {{invalid linear step: expected integral constant or variable reference}} */
  #pragma simd linear(j:k*k)
  for (int i = 0; i < 10; ++i) ;
  /* expected-error@+1 {{invalid linear step: expected integral constant or variable reference}} */
  #pragma simd linear(j:a.a)
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
  #pragma simd linear(j:0)
  for (int i = 0; i < 10; ++i);
  /* expected-warning@+1 {{linear step is zero}} */
  #pragma simd linear(j:C0)
  for (int i = 0; i < 10; ++i);
  /* expected-warning@+1 {{linear step is zero}} */
  #pragma simd linear(j:C1-C1)
  for (int i = 0; i < 10; ++i);
#pragma clang diagnostic pop
}
