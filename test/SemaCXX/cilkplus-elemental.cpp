// RUN: %clang_cc1 -DCXX11 -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s
// RUN: %clang_cc1 -DGNU   -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s
// RUN: %clang_cc1 -DMS    -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s

#ifdef GNU
# define ATTR(x)  __attribute__((x))
#endif
#ifdef MS
# define ATTR(x)  __declspec(x)
#endif
#ifndef ATTR
# define ATTR(x)  [[x]]
#endif

ATTR(vector(processor)) // expected-error {{attribute requires unquoted parameter}}
ATTR(vector(processor(ceci_nest_pas_un_processor))) // expected-error {{unrecognized processor}}
ATTR(vector(processor(pentium_4), processor(pentium_4_sse3))) // expected-warning {{inconsistent processor attribute}} \
                                                            // expected-note {{previous attribute is here}}
int test_processor_1(int x);


ATTR(vector(vectorlengthfor)) // expected-error {{attribute takes one argument}}
ATTR(vector(vectorlengthfor(int), vectorlengthfor(float))) // expected-warning {{inconsistent vectorlengthfor attribute}} \
                                                         // expected-note {{previous attribute is here}}
int test_vectorlengthfor_1(int x);

ATTR(vector(vectorlength)) // expected-error {{attribute takes one argument}}
ATTR(vector(vectorlength(-1))) // expected-error {{vectorlength must be positive}}
ATTR(vector(vectorlength(0)))  // expected-error {{vectorlength must be positive}}
ATTR(vector(vectorlength(int))) // expected-error {{attribute takes one argument}}
ATTR(vector(vectorlength(4), vectorlength(8))) // expected-warning {{repeated vectorlength attribute}} \
                                             // expected-note {{previous attribute is here}}
int test_vectorlength_1(int x);

ATTR(vector(uniform)) // expected-error {{attribute requires unquoted parameter}}
ATTR(vector(uniform(1))) // expected-error {{expected identifier}}
ATTR(vector(uniform(w))) // expected-error {{not a function parameter}}
ATTR(vector(uniform(z))) // expected-warning {{uniform parameter must have integral or pointer type}}
int test_uniform_1(int x, float z);

ATTR(vector(linear)) // expected-error {{attribute requires unquoted parameter}}
ATTR(vector(linear(1))) // expected-error {{expected identifier}}
ATTR(vector(linear(w))) // expected-error {{not a function parameter}}
ATTR(vector(linear(x:)))             // expected-error {{expected expression}}
ATTR(vector(linear(x), linear(x:2))) // expected-error {{linear attribute inconsistent with previous linear attribute}} \
                                   // expected-note {{previous attribute is here}}
ATTR(vector(linear(x), uniform(x)))  // expected-error {{linear attribute inconsistent with previous uniform attribute}} \
                                   // expected-note {{previous attribute is here}}
ATTR(vector(linear(x:w)))            // expected-error {{not a function parameter}}
int test_linear_1(int x, int y, float z);

ATTR(vector(mask(1))) // expected-error {{attribute takes no arguments}}
int test_mask_1(int x);
