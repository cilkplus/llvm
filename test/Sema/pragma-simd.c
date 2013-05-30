// RUN: %clang_cc1 -fsyntax-only -verify %s

int I();
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
