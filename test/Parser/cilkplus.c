// RUN: %clang_cc1 -fsyntax-only -verify %s

void keyword() {
  _Cilk_spawn; // expected-error {{Cilk Plus support disabled - compile with -fcilkplus}}
  _Cilk_sync; // expected-error {{Cilk Plus support disabled - compile with -fcilkplus}}
  _Cilk_for; // expected-error {{Cilk Plus support disabled - compile with -fcilkplus}}
  _Cilk_spawn keyword(); // expected-error {{Cilk Plus support disabled - compile with -fcilkplus}}
  _Cilk_for (i = 0; i < 10; ++i); // expected-error {{Cilk Plus support disabled - compile with -fcilkplus}}
}
