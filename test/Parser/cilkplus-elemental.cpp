// RUN: %clang_cc1 -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s

class X {
  __attribute__((vector(linear(this)))) // OK
  __attribute__((vector(uniform(this)))) // OK
  __attribute__((vector(linear(goto)))) // expected-error {{expected identifier or this}}
  __attribute__((vector(uniform(goto)))) // expected-error {{expected identifier or this}}
  void member();
};

__declspec(vector)               // OK
__declspec(vector())             // OK
__declspec(vector noinline)      // OK
__declspec(noinline vector)      // OK
__attribute__((vector))          // OK
__attribute__((vector()))        // OK
__attribute__((vector noinline)) // OK
__attribute__((noinline vector)) // OK
void test_vector();
