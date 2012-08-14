// RUN: %clang_cc1 -fsyntax-only -verify -fcilkplus %s

int _Cilk_spawn; // expected-error {{expected identifier}}
int _Cilk_sync; // expected-error {{expected identifier}}
int _Cilk_for; // expected-error {{expected identifier}}
