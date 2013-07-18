// RUN: %clang_cc1 -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s

[[vector(vectorlengthfor(1))]] // expected-error 2{{expected ')'}} \
                               // expected-note{{to match this '('}} \
                               // expected-error {{expected ']'}}
int test_vectorlengthfor_11(int x);

__attribute((vector(vectorlengthfor(2)))) // expected-error 2{{expected ')'}} \
                                          // expected-note{{to match this '('}}\
                                          // expected-error{{expected unqualified-id}}
int test_vectorlengthfor_12(int x);

__declspec(vector(vectorlengthfor(2))) // expected-error 2{{expected ')'}} \
                                       // expected-note{{to match this '('}}\
                                       // expected-error{{expected unqualified-id}}
int test_vectorlengthfor_13(int x);
