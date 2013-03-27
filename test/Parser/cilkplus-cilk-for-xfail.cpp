// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s
// XFAIL: *

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
