// RUN: %clang_cc1 -fsyntax-only -ferror-limit 0 -verify %s

/* expected-error@+1 {{expected for statement following '#pragma simd'}} */
#pragma simd

/* expected-error@+1 {{expected for statement following '#pragma simd'}} */
#pragma simd foo

/* expected-error@+1 {{expected for statement following '#pragma simd'}} */
#pragma simd vectorlength(4)

void test_no_clause()
{
  int i;
  #pragma simd
  for (i = 0; i < 16; ++i) ;
}

void test_invalid_clause()
{
  int i;
  /* expected-error@+1 {{invalid pragma simd clause}} */
  #pragma simd foo bar
  for (i = 0; i < 16; ++i) ;
}

void test_vectorlength()
{
  int i;
  /* expected-error@+1 {{invalid vectorlength expression: must be a power of two}} */
  #pragma simd vectorlength(3)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected '('}} */
  #pragma simd vectorlength
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected expression}} */
  #pragma simd vectorlength(
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected expression}} */
  #pragma simd vectorlength()
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected expression}} */
  #pragma simd vectorlength(,
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected expression}} */
  #pragma simd vectorlength(,)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected '('}} */
  #pragma simd vectorlength 4)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4,
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4,)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected expression}} */
  #pragma simd vectorlength(,4)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4 4)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4,,4)
  for (i = 0; i < 16; ++i) ;
  #pragma simd vectorlength(4)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4,8)
  for (i = 0; i < 16; ++i) ;
}

void test_vectorlengthfor()
{
  int i;
  /* expected-error@+1 {{expected a type}} */
  #pragma simd vectorlengthfor()
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected a type}} */
  #pragma simd vectorlengthfor(
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected a type}} */
  #pragma simd vectorlengthfor()
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{unknown type name 'foo'}} */
  #pragma simd vectorlengthfor(foo)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected a type}} */
  #pragma simd vectorlengthfor(0)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlengthfor(float,)
  for (i = 0; i < 16; ++i) ;
  #pragma simd vectorlengthfor(float)
  for (i = 0; i < 16; ++i) ;
}

void test_linear()
{
  int i;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(,
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(,)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear()
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(int)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(0)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{use of undeclared identifier 'x'}} */
  #pragma simd linear(x)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{use of undeclared identifier 'x'}} */
  /* expected-error@+1 {{use of undeclared identifier 'y'}} */
  #pragma simd linear(x, y)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+3 {{use of undeclared identifier 'x'}} */
  /* expected-error@+2 {{use of undeclared identifier 'y'}} */
  /* expected-error@+1 {{use of undeclared identifier 'z'}} */
  #pragma simd linear(x, y, z)
  for (i = 0; i < 16; ++i) ;

  int x, y;
  /* expected-error@+1 {{expected expression}} */
  #pragma simd linear(x:)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected expression}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(x:,)
  for (i = 0; i < 16; ++i) ;
  #pragma simd linear(x:1)
  for (i = 0; i < 16; ++i) ;
  #pragma simd linear(x:2*2)
  for (i = 0; i < 16; ++i) ;
  #pragma simd linear(x:1,y)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{use of undeclared identifier 'z'}} */
  #pragma simd linear(x:1,y,z:1)
  for (i = 0; i < 16; ++i) ;
}

void test_private()
{
  int i;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private(
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private(,
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private(,)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private()
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private(int)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private(0)
  for (i = 0; i < 16; ++i) ;
  #pragma simd private(x)
  for (i = 0; i < 16; ++i) ;
  #pragma simd private(x, y)
  for (i = 0; i < 16; ++i) ;
  #pragma simd private(x, y, z)
  for (i = 0; i < 16; ++i) ;
}

void test_firstprivate()
{
  int i;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate(
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate(,
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate(,)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate()
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate(int)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate(0)
  for (i = 0; i < 16; ++i) ;
  #pragma simd firstprivate(x)
  for (i = 0; i < 16; ++i) ;
  #pragma simd firstprivate(x, y)
  for (i = 0; i < 16; ++i) ;
  #pragma simd firstprivate(x, y, z)
  for (i = 0; i < 16; ++i) ;
}

void test_lastprivate()
{
  int i;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate(
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate(,
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate(,)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate()
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate(int)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate(0)
  for (i = 0; i < 16; ++i) ;
  #pragma simd lastprivate(x)
  for (i = 0; i < 16; ++i) ;
  #pragma simd lastprivate(x, y)
  for (i = 0; i < 16; ++i) ;
  #pragma simd lastprivate(x, y, z)
  for (i = 0; i < 16; ++i) ;
}

void test_reduction()
{
  int i;
  /* expected-error@+1 {{expected reduction operator}} */
  #pragma simd reduction(
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected reduction operator}} */
  #pragma simd reduction()
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected reduction operator}} */
  #pragma simd reduction(x)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected reduction operator}} */
  #pragma simd reduction(:x)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+2 {{expected reduction operator}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd reduction(,
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected ':'}} */
  #pragma simd reduction(+
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected ':'}} */
  #pragma simd reduction(+)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd reduction(+:
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd reduction(+:)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd reduction(+:,y)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd reduction(+:x,+:y)
  for (i = 0; i < 16; ++i) ;
  /* expected-error@+1 {{expected reduction operator}} */
  #pragma simd reduction(%:x)
  for (i = 0; i < 16; ++i) ;

  #pragma simd reduction(+:x)
  for (i = 0; i < 16; ++i) ;
  #pragma simd reduction(*:x)
  for (i = 0; i < 16; ++i) ;
  #pragma simd reduction(-:x)
  for (i = 0; i < 16; ++i) ;
  #pragma simd reduction(&:x)
  for (i = 0; i < 16; ++i) ;
  #pragma simd reduction(|:x)
  for (i = 0; i < 16; ++i) ;
  #pragma simd reduction(^:x)
  for (i = 0; i < 16; ++i) ;
  #pragma simd reduction(&&:x)
  for (i = 0; i < 16; ++i) ;
  #pragma simd reduction(||:x)
  for (i = 0; i < 16; ++i) ;
}

void test_multiple_clauses()
{
  int i;
  float x = 0, y = 0, z = 0;
  #pragma simd vectorlength(4) reduction(+:x, y) reduction(-:z)
  for (i = 0; i < 16; ++i) ;
}

void test_for()
{
  // expected-error@+3 {{expected '(' after 'for'}}
  // expected-error@+2 2{{use of undeclared identifier 'i'}}
  #pragma simd
  for int i = 0; i < 16; i++);

  // expected-error@+3 {{expected ')'}}
  // expected-note@+2 {{to match this '('}}
  #pragma simd
  for (int i = 0; i < 16; i++;

  // expected-error@+2 {{expected ';' in simd for}}
  #pragma simd
  for (int i = 0 i < 16; i++);

  // expected-error@+2 {{expected ';' in simd for}}
  #pragma simd
  for (int i = 0; i < 16 i++);

  // expected-error@+2 2{{expected ';' in simd for}}
  #pragma simd
  for (int i = 0 i < 16 i++);

  int i = 0;
  // expected-error@+2 {{missing initialization in simd for}}
  #pragma simd
  for (; i < 16; ++i);

  // expected-error@+2 {{missing loop condition expression in simd for}}
  #pragma simd
  for (int i = 0; ; ++i);

  // expected-error@+2 {{missing loop increment expression in simd for}}
  #pragma simd
  for (int i = 0; i < 16; );

  // expected-error@+3 {{missing loop condition expression in simd for}}
  // expected-error@+2 {{missing loop increment expression in simd for}}
  #pragma simd
  for (int i = 0; );
}
