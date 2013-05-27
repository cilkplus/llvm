// RUN: %clang_cc1 -fsyntax-only -ferror-limit 0 -verify %s

/* expected-warning@+1 {{expected loop statement following '#pragma simd' - ignored}} */
#pragma simd

/* expected-warning@+1 {{expected loop statement following '#pragma simd' - ignored}} */
#pragma simd foo

/* expected-warning@+1 {{expected loop statement following '#pragma simd' - ignored}} */
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
  /* expected-error@+1 {{Invalid vectorlength expression: Must be a power of two}} */
  #pragma simd vectorlength(3)
  /* expected-error@+1 {{expected '('}} */
  #pragma simd vectorlength
  /* expected-error@+1 {{expected expression}} */
  #pragma simd vectorlength(
  /* expected-error@+1 {{expected expression}} */
  #pragma simd vectorlength()
  /* expected-error@+1 {{expected expression}} */
  #pragma simd vectorlength(,
  /* expected-error@+1 {{expected expression}} */
  #pragma simd vectorlength(,)
  /* expected-error@+1 {{expected '('}} */
  #pragma simd vectorlength 4)
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4,
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4,)
  /* expected-error@+1 {{expected expression}} */
  #pragma simd vectorlength(,4)
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4 4)
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlength(4,,4)
  #pragma simd vectorlength(4)
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
  /* expected-error@+1 {{expected a type}} */
  #pragma simd vectorlengthfor(
  /* expected-error@+1 {{expected a type}} */
  #pragma simd vectorlengthfor()
  /* expected-error@+1 {{unknown type name 'foo'}} */
  #pragma simd vectorlengthfor(foo)
  /* expected-error@+1 {{expected a type}} */
  #pragma simd vectorlengthfor(0)
  /* expected-error@+2 {{expected ')'}} */
  /* expected-note@+1 {{to match this '('}} */
  #pragma simd vectorlengthfor(float,)
  #pragma simd vectorlengthfor(float)
  for (i = 0; i < 16; ++i) ;
}

void test_linear()
{
  int i;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(,
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(,)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear()
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(int)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(0)
  #pragma simd linear(x)
  #pragma simd linear(x, y)
  #pragma simd linear(x, y, z)
  for (i = 0; i < 16; ++i) ;

  /* expected-error@+1 {{expected expression}} */
  #pragma simd linear(x:)
  /* expected-error@+2 {{expected expression}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd linear(x:,)
  #pragma simd linear(x:1)
  #pragma simd linear(x:2*2)
  #pragma simd linear(x:1,y)
  #pragma simd linear(x:1,y,z:1)
  for (i = 0; i < 16; ++i) ;
}

void test_private()
{
  int i;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private(
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private(,
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private(,)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private()
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private(int)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd private(0)
  #pragma simd private(x)
  #pragma simd private(x, y)
  #pragma simd private(x, y, z)
  for (i = 0; i < 16; ++i) ;
}

void test_firstprivate()
{
  int i;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate(
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate(,
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate(,)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate()
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate(int)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd firstprivate(0)
  #pragma simd firstprivate(x)
  #pragma simd firstprivate(x, y)
  #pragma simd firstprivate(x, y, z)
  for (i = 0; i < 16; ++i) ;
}

void test_lastprivate()
{
  int i;
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate(
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate(,
  /* expected-error@+2 {{expected identifier}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate(,)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate()
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate(int)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd lastprivate(0)
  #pragma simd lastprivate(x)
  #pragma simd lastprivate(x, y)
  #pragma simd lastprivate(x, y, z)
  for (i = 0; i < 16; ++i) ;
}

void test_reduction()
{
  int i;
  /* expected-error@+1 {{expected reduction operator}} */
  #pragma simd reduction(
  /* expected-error@+1 {{expected reduction operator}} */
  #pragma simd reduction()
  /* expected-error@+1 {{expected reduction operator}} */
  #pragma simd reduction(x)
  /* expected-error@+1 {{expected reduction operator}} */
  #pragma simd reduction(:x)
  /* expected-error@+2 {{expected reduction operator}} */
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd reduction(,
  /* expected-error@+1 {{expected ':'}} */
  #pragma simd reduction(+
  /* expected-error@+1 {{expected ':'}} */
  #pragma simd reduction(+)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd reduction(+:
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd reduction(+:)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd reduction(+:,y)
  /* expected-error@+1 {{expected identifier}} */
  #pragma simd reduction(+:x,+:y)
  /* expected-error@+1 {{expected reduction operator}} */
  #pragma simd reduction(%:x)
  for (i = 0; i < 16; ++i) ;

  #pragma simd reduction(+:x)
  #pragma simd reduction(*:x)
  #pragma simd reduction(-:x)
  #pragma simd reduction(&:x)
  #pragma simd reduction(|:x)
  #pragma simd reduction(^:x)
  #pragma simd reduction(&&:x)
  #pragma simd reduction(||:x)
  for (i = 0; i < 16; ++i) ;
}

void test_multiple_clauses()
{
  int i;
  float x = 0, y = 0, z = 0;
  #pragma simd vectorlength(4) reduction(+:x, y) reduction(-:z)
  for (i = 0; i < 16; ++i)
    ;
}

void test_for()
{
}
