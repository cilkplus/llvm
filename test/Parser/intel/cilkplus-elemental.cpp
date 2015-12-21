// RUN: %clang_cc1 -std=c++11 -DCXX11 -triple x86_64-pc-linux-gnu -fcilkplus -fsyntax-only -verify -Wall  %s
// RUN: %clang_cc1 -DGNU -triple x86_64-pc-linux-gnu -fcilkplus -fsyntax-only -verify -Wall  %s
// RUN: %clang_cc1 -DMS -fms-extensions -triple x86_64-pc-win32 -fcilkplus -fsyntax-only -verify -Wall  %s
// REQUIRES: cilkplus

#ifdef GNU
# define ATTR(x) __attribute__((x))
#endif
#ifdef MS
# define ATTR(x) __declspec(x)
#endif
#ifndef ATTR
# define ATTR(x)  [[x]]
#endif

class X {
  ATTR(vector(uniform(this))) // OK
  ATTR(vector(linear(goto))) // expected-error {{expected identifier or this}}
  ATTR(vector(uniform(goto))) // expected-error {{expected identifier or this}}
  void member();
};

ATTR(vector)            // OK
ATTR(vector())          // OK
void test_vector();
