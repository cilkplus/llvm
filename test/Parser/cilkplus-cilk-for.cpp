// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s
// XFAIL: *

_Cilk_for; // expected-error {{expected unqualified-id}}

int k = _Cilk_for; // expected-error {{expected expression}}

void f1() {
  _Cilk_for (int i = 0; i < 10; i++); // OK

  _Cilk_for int i = 0; i < 10; i++); // expected-error {{expected '(' after '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; i++; // expected-error {{expected ')'}}

  _Cilk_for (int i = 0 i < 10; i++); // expected-error {{expected ';' in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10 i++); // expected-error {{expected ';' in '_Cilk_for'}}

  _Cilk_for (int i = 0 i < 10 i++); // expected-error 2{{expected ';' in '_Cilk_for'}}
}

void f2() {
  #pragma cilk grainsize = 4
  _Cilk_for (int i = 0; i < 10; i++); // OK

  #pragma cilk grainsize = 4
  for (int i = 0; i < 10; i++); // expected-error {{'#pragma cilk' must be followed by a '_Cilk_for' loop}}

  /* expected-error {{expected 'grainsize' in '#pragma cilk'}} */ #pragma cilk
  _Cilk_for (int i = 0; i < 10; i++);

  /* expected-error {{expected '=' in '#pragma cilk'}} */ #pragma cilk grainsize
  _Cilk_for (int i = 0; i < 10; i++);

  /* expected-error {{expected '=' in '#pragma cilk'}} */ #pragma cilk grainsize 4
  _Cilk_for (int i = 0; i < 10; i++);

  /* expected-error {{unknown pragma ignored}} */ #pragma grainsize = 4
  _Cilk_for (int i = 0; i < 10; i++);

  /* expected-warning {{extra tokens at end of '#pragma cilk' - ignored}} */ #pragma cilk grainsize = 4;
  _Cilk_for (int i = 0; i < 10; i++);
}
