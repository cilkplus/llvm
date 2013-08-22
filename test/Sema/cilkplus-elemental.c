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
ATTR(vector(vectorlength(VL)))      // expected-error {{vectorlength attribute requires an integer constant}}
ATTR(vector(vectorlength(VL + 2)))  // expected-error {{vectorlength attribute requires an integer constant}}
int test_vectorlength_1(int x);

ATTR(vector(linear(x:y))) // expected-error {{linear step parameter must also be uniform}}
ATTR(vector(uniform(y)))
int test_step_1(int x, int y);

ATTR(vector(uniform(y), linear(x:y)))
ATTR(vector(linear(x:y))) // expected-error {{linear step parameter must also be uniform}}
ATTR(vector(linear(x:y), uniform(y)))
int test_step_1(int x, int y);

struct S {};
ATTR(vector(uniform(s))) // OK
ATTR(vector(linear(s)))  // expected-error {{linear parameter type 'struct S' is not an integer or pointer type}}
ATTR(vector(linear(i)))  // OK
int test_int_ptr(struct S s, int i);

ATTR(vector(uniform(this)))               // OK
ATTR(vector(linear(this)))                // OK
ATTR(vector(uniform(this, this)))         // expected-error {{parameter 'this' cannot be the subject of two elemental clauses}} // expected-note{{here}}
ATTR(vector(linear(this, this)))          // expected-error {{parameter 'this' cannot be the subject of two elemental clauses}} // expected-note{{here}}
ATTR(vector(uniform(this), linear(this))) // expected-error {{parameter 'this' cannot be the subject of two elemental clauses}} // expected-note{{here}}
ATTR(vector(linear(this), uniform(this))) // expected-error {{parameter 'this' cannot be the subject of two elemental clauses}} // expected-note{{here}}
int test_this(int this); // expected-note 4{{parameter here}}
