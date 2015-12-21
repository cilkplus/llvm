// RUN: %clang_cc1 -fcilkplus -o %t -verify %s
// expected-no-diagnostics
// REQUIRES: cilkplus

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

}

struct A { int a; };

#define SMALL_FLOAT_ARRAY_SIZE 100
#define N SMALL_FLOAT_ARRAY_SIZE
float a[N];
int e = 1;
int f = 2;

int
check ()
{
  int i;
  int l = 0, ef = sizeof(int) == 4?f:e;
#pragma simd linear(l:ef)
  for (i = 0; i < N; i++)
    {
      a[i] = (l += (sizeof(int) == 4?f:e));
    }
  return l;
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

}

void test_init() {
  int i, j;

  #pragma simd
  for (i = 0; i < 10; ++i); // OK

  #pragma simd
  for (((i) = 0); i < 10; ++i); // OK

}

void test_pragma_simd() {
  int j, k, longvar;

  extern int e;

  #pragma simd linear(k) linear(j)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd linear(k:j) linear(e:j)
  for (int i = 0; i < 10; ++i) ;

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

  #pragma simd
  for (int i = 0; i < 10; (++i)); // OK
  int j = 2;
  #pragma simd
  for (int i = 0; i < 10; i -= j); // OK
  #pragma simd
  for (int i = 0; i < 10; i -= -1); // OK

}

void empty_body() {
  #pragma simd
  for (int i = 0; i < 10; ++i) // OK
    ;
  { }

  #pragma simd
  for (int i = 0; i < 10; ++i); // OK
  empty_body();
}

