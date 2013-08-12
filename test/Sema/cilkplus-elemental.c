// RUN: %clang_cc1 -DGNU -fcilkplus -fsyntax-only -verify -Wall %s
// RUN: %clang_cc1 -DMS  -fcilkplus -fsyntax-only -verify -Wall %s

#ifdef GNU
# define ATTR(x)  __attribute__((x))
#endif
#ifdef MS
# define ATTR(x)  __declspec(x)
#endif

// vectorlength clause
const int VL = 2;
ATTR(vector(vectorlength(VL)))      // expected-error {{'vectorlength' attribute requires integer constant}}
ATTR(vector(vectorlength(VL + 2)))  // expected-error {{'vectorlength' attribute requires integer constant}}
int test_vectorlength_1(int x);

ATTR(vector(linear(x:y))) // expected-error {{linear step parameter must also be uniform}}
ATTR(vector(uniform(y)))
int test_step_1(int x, int y);

ATTR(vector(uniform(y), linear(x:y)))
ATTR(vector(linear(x:y))) // expected-error {{linear step parameter must also be uniform}}
ATTR(vector(linear(x:y), uniform(y)))
int test_step_1(int x, int y);
