// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify -Wall %s

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

void f2() {
  #pragma cilk grainsize = 4
  _Cilk_for (int i = 0; i < 10; i++); // OK

  extern int foo(int);
  #define FOO(x) foo(x)

  #pragma cilk grainsize = FOO(10) // OK, with macros.
  _Cilk_for (int i = 0; i < 10; i++);

  #pragma cilk grainsize = 4
  for (int i = 0; i < 10; i++); // expected-error {{'#pragma cilk' must be followed by a '_Cilk_for' loop}}

  #pragma cilk /* expected-warning {{expected identifier in '#pragma cilk' - ignored}} */
  _Cilk_for (int i = 0; i < 10; i++);

  #pragma cilk grainsize /* expected-error {{expected '=' in '#pragma cilk'}} */
  _Cilk_for (int i = 0; i < 10; i++);

  #pragma cilk grainsize 4 /* expected-error {{expected '=' in '#pragma cilk'}} */
  _Cilk_for (int i = 0; i < 10; i++);

  #pragma grainsize = 4 /* expected-warning {{unknown pragma ignored}} */
  _Cilk_for (int i = 0; i < 10; i++)
  ;

  #pragma cilk grainsize = 4; /* expected-warning {{extra tokens at end of '#pragma cilk' - ignored}} */
  _Cilk_for (int i = 0; i < 10; i++);
}
