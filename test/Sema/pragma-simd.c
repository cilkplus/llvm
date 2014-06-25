// RUN: %clang_cc1 -fsyntax-only -verify %s

int I();
const int C0 = 0;
const int C1 = 1;
const int C2 = 2;
const char CC = (char)32;
const short CS = (short)32;
void test_vectorlength() {
  int i, j;
  #pragma simd vectorlength(4)
  for (i = 0; i < 10; ++i) ;

  /* expected-error@+1 {{vectorlength expression must be an integer constant}} */
  #pragma simd vectorlength(CC)
  /* expected-error@+1 {{vectorlength expression must be an integer constant}} */
  #pragma simd vectorlength(CS)
  /* expected-error@+1 {{vectorlength expression must be an integer constant}} */
  #pragma simd vectorlength(C1)
  /* expected-error@+1 {{vectorlength expression must be an integer constant}} */
  #pragma simd vectorlength(C2+C2)
  /* expected-error@+1 {{vectorlength expression must be an integer constant}} */
  #pragma simd vectorlength(j)
  /* expected-error@+1 {{vectorlength expression must be an integer constant}} */
  #pragma simd vectorlength(C1+j)
  /* expected-error@+1 {{vectorlength expression must be an integer constant}} */
  #pragma simd vectorlength(I())
  for (i = 0; i < 10; ++i) ;
}

struct A { int a; };

void test_reduction() {
  struct A A;
  int a, i;
  #pragma simd reduction(max:a)
  for (i = 0; i < 10; ++i);
  /* expected-error@+1 {{reduction operator max requires arithmetic type}} */
  #pragma simd reduction(max:A)
  for (i = 0; i < 10; ++i);
  /* expected-error@+1 {{reduction operator min requires arithmetic type}} */
  #pragma simd reduction(min:A)
  for (i = 0; i < 10; ++i);
  int arr[100]; // expected-note {{declared here}}
  /* expected-error@+1 {{variable in reduction clause shall not be of array type}} */
  #pragma simd reduction(+:arr)
  for (i = 0; i < 10; ++i);
  const int c = 10; // expected-note {{declared here}}
  /* expected-error@+1 {{variable in reduction clause shall not be const-qualified}} */
  #pragma simd reduction(+:c)
  for (i = 0; i < 10; ++i);
  /* expected-error@+1 {{invalid reduction variable}} */
  #pragma simd reduction(+:test_reduction)
  for (i = 0; i < 10; ++i);
}

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
  /* expected-error@+1 {{cannot specify multiple vectorlength clauses}} */
  #pragma simd vectorlength(4) vectorlength(8)
  for (int i = 0; i < 10; ++i) ;

  struct A A;
  /* expected-error@+1 {{reduction variable shall not appear in multiple simd clauses}} */
  #pragma simd reduction(max:k) reduction(min:k) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{reduction variable shall not appear in multiple simd clauses}} */
  #pragma simd reduction(+:A.a) reduction(-:A.a) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{linear variable shall not appear in multiple simd clauses}} */
  #pragma simd reduction(min:k) linear(k) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{reduction variable shall not appear in multiple simd clauses}} */
  #pragma simd linear(k) reduction(min:k) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{reduction variable shall not appear in multiple simd clauses}} */
  #pragma simd reduction(max:k,k) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);

  int x;
  /* expected-error@+1 {{private variable shall not appear in multiple simd clauses}} */
  #pragma simd linear(x) private(x) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{linear variable shall not appear in multiple simd clauses}} */
  #pragma simd private(x) linear(x) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{private variable shall not appear in multiple simd clauses}} */
  #pragma simd reduction(+:x) private(x) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{reduction variable shall not appear in multiple simd clauses}} */
  #pragma simd private(x) reduction(+:x) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{firstprivate variable shall not appear in multiple simd clauses}} */
  #pragma simd private(x) firstprivate(x) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{private variable shall not appear in multiple simd clauses}} */
  #pragma simd firstprivate(x) private(x) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{lastprivate variable shall not appear in multiple simd clauses}} */
  #pragma simd private(x) lastprivate(x) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  /* expected-error@+1 {{private variable shall not appear in multiple simd clauses}} */
  #pragma simd lastprivate(x) private(x) // expected-note {{first used here}}
  for (int i = 0; i < 10; ++i);
  #pragma simd lastprivate(x) firstprivate(x)
  for (int i = 0; i < 10; ++i);
  #pragma simd firstprivate(x) lastprivate(x)
  for (int i = 0; i < 10; ++i);
}

void test_condition() {
  int j = 1;

  #pragma simd
  for (int i = 0; i <= 10; ++i); // OK

  #pragma simd
  for (int i = 0; i != 10; ++i); // OK

  #pragma simd
  for (int i = 10; i > 0; --i); // OK

  #pragma simd
  for (int i = 10; i >= 0; --i); // OK

  #pragma simd
  for (int i = 10; i >= (j << 2); --i); // OK

  #pragma simd
  for (int i = 0; 10 >= i; ++i); // OK

  #pragma simd
  for (int i = 0; 10 != i; ++i); // OK

  #pragma simd
  for (int i = 10; 0 < i; --i); // OK

  #pragma simd
  for (int i = 10; 0 <= i; --i); // OK

  #pragma simd
  for (int i = 10; (j << 2) <= i; --i); // OK

  #pragma simd
  for (char i = 0; i < 10; ++i); // OK

  #pragma simd
  for (short i = 0; i < 10; ++i); // OK

  #pragma simd
  for (long i = 0; i < 10; ++i); // OK

  // expected-error@+2 {{expected binary comparison operator in simd for loop condition}}
  #pragma simd
  for (int i = 0; i; ++i);

  // expected-error@+2 {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in simd for}}
  #pragma simd
  for (int i = 0; i & 0x20; ++i);

  // expected-error@+2 {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in simd for}}
  #pragma simd
  for (int i = 0; k >> i; ++i);

  // expected-error@+2 {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in simd for}}
  #pragma simd
  for (int i = 0; i == 10; ++i);

  // expected-error@+3 {{loop condition does not test control variable 'i' in simd for}}
  // expected-note@+2 {{allowed forms are 'i' OP expr, and expr OP 'i'}}
  #pragma simd
  for (int i = 0; (i << 1) < 10; ++i);

  // expected-error@+3 {{loop condition does not test control variable 'i' in simd for}}
  // expected-note@+2 {{allowed forms are 'i' OP expr, and expr OP 'i'}}
  #pragma simd
  for (int i = 0; 10 > (i << 1); ++i);

  // expected-error@+3 {{loop condition does not test control variable 'i' in simd for}}
  // expected-note@+2 {{allowed forms are 'i' OP expr, and expr OP 'i'}}
  #pragma simd
  for (int i = 0; j < 10; ++i);
}

void test_increment() {
  #pragma simd
  for (int i = 0; i < 10; ++i); // OK
  #pragma simd
  for (int i = 0; i < 10; i++); // OK
  #pragma simd
  for (int i = 10; i > 0; --i); // OK
  #pragma simd
  for (int i = 10; i > 0; i--); // OK
  #pragma simd
  for (int i = 10; i > 0; i -= 2); // OK
  #pragma simd
  for (int i = 0; i < 10; i += 2); // OK

  extern int next();
  #pragma simd
  for (int i = 0; i < 10; i += next()); // OK

  enum E { a = 0, b = 5 };
  #pragma simd
  for (int i = 0; i < 10; i += b); // OK

  // expected-error@+2 {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in simd for}}
  #pragma simd
  for (int i = 0; i < 10; !i);
  // expected-error@+2 {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in simd for}}
  #pragma simd
  for (int i = 0; i < 10; i *= 2);

  // expected-error@+2 {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in simd for}}
  #pragma simd
  for (int i = 0; i < 10; i <<= 1);

  // expected-error@+2 {{right-hand side of '+=' must have integral or enum type in simd for increment}}
  #pragma simd
  for (int i = 0; i < 10; i += 1.2f);

  // expected-error@+2 {{right-hand side of '-=' must have integral or enum type in simd for increment}}
  #pragma simd
  for (int i = 10; i > 0; i -= 1.2f);

  // expected-warning@+3 {{expression result unused}}
  // expected-error@+2 {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in simd for}}
  #pragma simd
  for (int i = 0; i < 10; (0, ++i));

  #pragma simd
  for (int i = 0; i < 10; (i++)); // OK
  #pragma simd
  for (int i = 0; i < 10; ((i++))); // OK

  // expected-error@+2 {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in simd for}}
  #pragma simd
  for (int i = 0; i < 10; (i *= 2));

  // expected-error@+2 {{right-hand side of '+=' must have integral or enum type in simd for increment}}
  #pragma simd
  for (int i = 0; i < 10; (i += 1.2f));

  int j = -1;
  // expected-error@+2 {{loop increment does not modify control variable 'i' in simd for}}
  #pragma simd
  for (int i = 0; i < 10; j++);

  #pragma simd
  for (int i = 0; i < 10; (++i)); // OK
  #pragma simd
  for (int i = 0; i < 10; i -= j); // OK
  #pragma simd
  for (int i = 0; i < 10; i -= -1); // OK

  // Tests for inconsistency between loop condition and increment
  //
  // expected-note@+3 {{constant stride is -1}}
  // expected-error@+2 {{loop increment is inconsistent with condition in simd for: expected positive stride}}
  #pragma simd
  for (int i = 0; i < 10; i--);

  // expected-note@+3 {{constant stride is -1}}
  // expected-error@+2 {{loop increment is inconsistent with condition in simd for: expected positive stride}}
  #pragma simd
  for (int i = 0; i < 10; i -= 1);

  // expected-note@+3 {{constant stride is 1}}
  // expected-error@+2 {{loop increment is inconsistent with condition in simd for: expected negative stride}}
  #pragma simd
  for (int i = 10; i >= 0; i++);

  // expected-error@+2 {{loop increment must be non-zero in simd for}}
  #pragma simd
  for (int i = 10; i >= 0; i += 0);
}

void empty_body() {
  // expected-warning@+3 {{for loop has empty body}}
  // expected-note@+2 {{put the semicolon on a separate line to silence this warning}}
  #pragma simd
  for (int i = 0; i < 10; ++i);
  { }

  #pragma simd
  for (int i = 0; i < 10; ++i) // OK
    ;
  { }

  // expected-warning@+3 {{for loop has empty body}}
  // expected-note@+2 {{put the semicolon on a separate line to silence this warning}}
  #pragma simd
  for (int i = 0; i < 10; ++i);
    empty_body();

  #pragma simd
  for (int i = 0; i < 10; ++i); // OK
  empty_body();
}

void test_LCV() {
  int i;
  // expected-warning@+1 {{ignoring simd clause applied to simd loop control variable}}
  #pragma simd private(i)
  for (i = 0; i < 10; ++i);
  // expected-warning@+1 {{ignoring simd clause applied to simd loop control variable}}
  #pragma simd lastprivate(i)
  for (i = 0; i < 10; ++i);
  // expected-warning@+1 {{ignoring simd clause applied to simd loop control variable}}
  #pragma simd linear(i)
  for (i = 0; i < 10; ++i);

  // expected-error@+1{{the simd loop control variable may not be the subject of a firstprivate clause}}
  #pragma simd firstprivate(i)
  for (i = 0; i < 10; ++i);
  // FIXME: should be an error {{the simd loop control variable may not be the subject of a reduction clause}}
  #pragma simd reduction(+:i)
  for (i = 0; i < 10; ++i);
}
