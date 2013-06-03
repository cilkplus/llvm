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

void test_vectorlengthfor() {
  int i;
  #pragma simd vectorlengthfor(char)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(short)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(int)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(long)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(long long)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(unsigned long long)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(float)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(double)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(void *)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(void ****)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(void const *)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(void * const)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(void const * const)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(short *)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(int[45])
  for (i = 0; i < 10; ++i);
  enum E { A, B, C };
  #pragma simd vectorlengthfor(enum E)
  for (i = 0; i < 10; ++i);
  struct S { int a; int b; int c; };
  #pragma simd vectorlengthfor(struct S)
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(void(int, int))
  for (i = 0; i < 10; ++i);


  struct Incomplete;
  #pragma simd vectorlengthfor(struct Incomplete) // expected-error {{cannot select vector length for incomplete type 'struct Incomplete'}}
  for (i = 0; i < 10; ++i);
  typedef struct Incomplete I;
  #pragma simd vectorlengthfor(I) // expected-error {{cannot select vector length for incomplete type 'I' (aka 'struct Incomplete')}}
  for (i = 0; i < 10; ++i);
  #pragma simd vectorlengthfor(void) // expected-error {{cannot select vector length for void type}}
  for (i = 0; i < 10; ++i);
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

int k;
extern int m; // expected-note {{'m' declared here}}

void test_init() {
  int i, j;

  #pragma simd
  for (i = 0; i < 10; ++i); // OK

  #pragma simd
  for (((i) = 0); i < 10; ++i); // OK

  // expected-error@+2 {{cannot declare more than one loop control variable in simd for}}
  #pragma simd
  for (int i = 0, j = 0; i < 10; ++i);

  // expected-error@+2 {{cannot initialize more than one loop control variable in simd for}}
  #pragma simd
  for (i = 0, j = 0; i < 10; ++i);

  // expected-error@+2 {{simd for loop control variable must be initialized}}
  #pragma simd
  for (int i; i < 10; ++i);

  int a[1];
  // expected-error@+2 {{expect a loop control variable in simd for}}
  #pragma simd
  for (a[0] = 0; i < 10; i++);

  // expected-error@+2 {{expect a loop control variable in simd for}}
  #pragma simd
  for (*&i = 0; *&i < 10; (*&i)++);

  extern int *get_intptr();
  // expected-error@+2 {{expect a loop control variable in simd for}}
  #pragma simd
  for (*get_intptr() = 0; i < 10; i++);

  // expected-error@+2 {{loop control variable cannot have storage class 'auto' in simd for}}
  #pragma simd
  for (auto int i = 0; i < 10; ++i);

  // expected-error@+2 {{loop control variable cannot have storage class 'static' in simd for}}
  #pragma simd
  for (static int i = 0; i < 10; ++i);

  // expected-error@+2 {{loop control variable cannot have storage class 'register' in simd for}}
  #pragma simd
  for (register int i = 0; i < 10; ++i);

  // expected-error@+2 {{non-local loop control variable in simd for}}
  #pragma simd
  for (k = 0; k < 10; ++k);

  // expected-error@+2 {{loop control variable cannot have storage class 'extern' in simd for}}
  #pragma simd
  for (m = 0; m < 10; ++m);

  // expected-error@+2 {{loop control variable cannot be 'volatile' in simd for}}
  #pragma simd
  for (volatile int i = 0; i < 10; ++i);

  // expected-note@+1 {{'f' declared here}}
  float f;
  // expected-error@+2 {{loop control variable shall have an integer or pointer type}}
  #pragma simd
  for (f = 0.0f; f < 10.0f; ++f);

  // expected-note@+1 {{'u' declared here}}
  union { int i; void *p; } u, v;
  // expected-error@+2 {{expect a loop control variable in simd for}}
  #pragma simd
  for (u.i = 0; u.i < 10; u.i += 1);

  // expected-error@+2 {{loop control variable shall have an integer or pointer type}}
  #pragma simd
  for (u = v; u.i < 10; u.i += 1);
}

void test_pragma_simd() {
  int j, k, longvar;

  extern int e;

  #pragma simd linear(k) linear(j)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd linear(k:j) linear(e:j)
  for (int i = 0; i < 10; ++i) ;

  /* expected-note@+2 {{first used here}} */
  /* expected-error@+1 {{linear variable shall not be used as a linear step}} */
  #pragma simd linear(k:k)
  for (int i = 0; i < 10; ++i) ;
  /* expected-note@+2 {{first used here}} */
  /* expected-error@+1 {{linear variable shall not appear in multiple simd clauses}} */
  #pragma simd linear(k) linear(k)
  for (int i = 0; i < 10; ++i) ;
  /* expected-note@+2 {{first used here}} */
  /* expected-error@+1 {{linear variable shall not be used as a linear step}} */
  #pragma simd linear(j) linear(k:j)
  for (int i = 0; i < 10; ++i) ;
  /* expected-note@+2 {{first used here}} */
  /* expected-error@+1 {{linear variable shall not appear in multiple simd clauses}} */
  #pragma simd linear(k) linear(k:j)
  for (int i = 0; i < 10; ++i) ;
  /* expected-note@+2 {{first used here}} */
  /* expected-error@+1 {{linear variable shall not appear in multiple simd clauses}} */
  #pragma simd linear(longvar:j) linear(longvar)
  for (int i = 0; i < 10; ++i) ;
  /* expected-note@+2 {{first used here}} */
  /* expected-error@+1 {{linear step shall not be used as a linear variable}} */
  #pragma simd linear(k:j) linear(j)
  for (int i = 0; i < 10; ++i) ;
  /* expected-note@+2 {{first used here}} */
  /* expected-error@+1 {{linear variable shall not be used as a linear step}} */
  #pragma simd linear(e:e)
  for (int i = 0; i < 10; ++i) ;

  /* expected-note@+2 {{vectorlength first specified here}} */
  /* expected-error@+1 {{cannot specify both vectorlengthfor and vectorlength}} */
  #pragma simd vectorlength(8) vectorlengthfor(int)
  for (int i = 0; i < 10; ++i) ;
  /* expected-note@+2 {{vectorlengthfor first specified here}} */
  /* expected-error@+1 {{cannot specify both vectorlength and vectorlengthfor}} */
  #pragma simd vectorlengthfor(int) vectorlength(8)
  for (int i = 0; i < 10; ++i) ;
  /* expected-note@+4 {{vectorlengthfor first specified here}} */
  /* expected-error@+3 {{cannot specify multiple vectorlengthfor clauses}} */
  /* expected-note@+2 {{vectorlengthfor first specified here}} */
  /* expected-error@+1 {{cannot specify both vectorlength and vectorlengthfor}} */
  #pragma simd vectorlengthfor(int) vectorlength(8) vectorlengthfor(int)
  for (int i = 0; i < 10; ++i) ;
  /* expected-note@+2 {{vectorlength first specified here}} */
  /* expected-error@+1 {{cannot specify multiple vectorlength clauses}} */
  #pragma simd vectorlength(4) vectorlength(8)
  for (int i = 0; i < 10; ++i) ;
}
