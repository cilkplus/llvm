// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s

_Cilk_for; // expected-error {{expected unqualified-id}}

int k = _Cilk_for; // expected-error {{expected expression}}

void f1() {
  _Cilk_for (int i = 0; i < 10; i++); // OK

  _Cilk_for int i = 0; i < 10; i++); // expected-error {{expected '(' after '_Cilk_for'}} \
                                     // expected-error 2{{use of undeclared identifier 'i'}}

  _Cilk_for (int i = 0; i < 10; i++; // expected-error {{expected ')'}} \
                                     // expected-note {{to match this '('}}

  _Cilk_for (int i = 0 i < 10; i++); // expected-error {{expected ';' in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10 i++); // expected-error {{expected ';' in '_Cilk_for'}}

  _Cilk_for (int i = 0 i < 10 i++); // expected-error 2{{expected ';' in '_Cilk_for'}}

  int i = 0;
  _Cilk_for (; i < 10; ++i); // expected-error {{missing control variable declaration and initialization in '_Cilk_for'}}

  _Cilk_for (int i = 0; ; ++i); // expected-error {{missing loop condition expression in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; ); // expected-error {{missing loop increment expression in '_Cilk_for'}}

  _Cilk_for (int i = 0; ); // expected-error {{missing loop condition expression in '_Cilk_for'}} \
                           // expected-error {{missing loop increment expression in '_Cilk_for'}}
}
